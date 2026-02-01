#include "viewportRenderer.h"

#include <glm/ext/matrix_clip_space.hpp>

#include "backend/selection.h"
#include "gpu/renderer/viewport/elements/elementRenderer.h"
#include "gpu/mainRenderer.h"
#include "gpu/abstractions/vulkanImage.h"
#include "imgui/imgui_impl_vulkan.h"

ViewportRenderer::ViewportRenderer(VulkanDevice* device) : chunker(device), device(device) {
	createRenderPass();
	gridRenderer.init(device, renderPass);
	chunkRenderer.init(device, renderPass);
	elementRenderer.init(device, renderPass);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	vkCreateSampler(device->getDevice(), &samplerInfo, nullptr, &sampler);

	frames.init(device);

	running = true;
	renderThread = std::thread(&ViewportRenderer::renderLoop, this);
}

ViewportRenderer::~ViewportRenderer() {
	running = false;
	if (renderThread.joinable()) {
		renderThread.join();
	}

	destroyImages();

	vkDestroySampler(device->getDevice(), sampler, nullptr);
	vkDestroyRenderPass(device->getDevice(), renderPass, nullptr);

	elementRenderer.cleanup();
	chunkRenderer.cleanup();
	gridRenderer.cleanup();
}

ViewportViewData ViewportRenderer::getViewData() {
	std::lock_guard<std::mutex> lock(viewMux);
	return viewData;
}

// ====================================== INTERFACE ==========================================

void ViewportRenderer::setSimulator(const EvalLogicSimulator* simulator, const Address& address) {
	std::lock_guard<std::mutex> lock1(simulatorMux);
	std::lock_guard<std::mutex> lock2(addressMux);
	this->simulator = simulator;
	this->address = address;
	chunker.setSimulator(simulator, address);
}

void ViewportRenderer::updateViewFrame(glm::vec2 size) {
	std::lock_guard<std::mutex> lock(viewMux);

	// Only recreate if size actually changed
	if (newViewData.viewportSize.width != static_cast<uint32_t>(size.x) ||
	    newViewData.viewportSize.height != static_cast<uint32_t>(size.y)) {
		newViewData.viewportSize.width = size.x;
		newViewData.viewportSize.height = size.y;
		recreateImages = true;
	}
}

void ViewportRenderer::updateView(FPosition topLeft, FPosition bottomRight) {
	std::lock_guard<std::mutex> lock(viewMux);
	newViewData.viewportViewMat = glm::ortho(topLeft.x, bottomRight.x, topLeft.y, bottomRight.y);
	newViewData.viewBounds = { topLeft, bottomRight };
	float viewWidth = bottomRight.x - topLeft.x;
	float viewHeight = bottomRight.y - topLeft.y;
	newViewData.viewScale = std::min(viewWidth, viewHeight);
}

VkDescriptorSet ViewportRenderer::getLatestImage() {
	std::lock_guard<std::mutex> lock(imageMux);
	// Return the texture that ImGui should read from
	// Make sure images are actually created and ready
	if (!imagesReady || imguiTextures.empty() || imguiReadTexture == VK_NULL_HANDLE) {
		return VK_NULL_HANDLE;
	}
	return imguiReadTexture;
}

ElementId ViewportRenderer::addSelectionObjectElement(const SelectionObjectElement& selection) {
	std::lock_guard<std::mutex> lock(elementsMux);

	ElementId newElement = ++currentElementId;

	std::stack<SharedSelection> selectionsLeft;
	selectionsLeft.push(selection.selection);
	while (!selectionsLeft.empty()) {
		SharedSelection selectionObj = selectionsLeft.top();
		selectionsLeft.pop();

		SharedCellSelection cellSelection = selectionCast<CellSelection>(selectionObj);
		if (cellSelection) {
			BoxSelectionRenderData newBoxSelection;
			Position position = cellSelection->getPosition();

			newBoxSelection.topLeft = glm::vec2(position.x, position.y);
			newBoxSelection.size = glm::vec2(1.0f);
			if (selection.renderMode == SelectionObjectElement::RenderMode::SELECTION) newBoxSelection.state = BoxSelectionRenderData::Normal;
			else if (selection.renderMode == SelectionObjectElement::RenderMode::SELECTION_INVERTED) newBoxSelection.state = BoxSelectionRenderData::Inverted;
			else if (selection.renderMode == SelectionObjectElement::RenderMode::ARROWS) newBoxSelection.state = BoxSelectionRenderData::Special;

			boxSelections[newElement].push_back(newBoxSelection);
		}

		SharedDimensionalSelection dimensionalSelection = selectionCast<DimensionalSelection>(selectionObj);
		if (dimensionalSelection) {
			if (selection.renderMode != SelectionObjectElement::RenderMode::ARROWS) {
				for (int i = 0; i < dimensionalSelection->size(); i++) {
					selectionsLeft.push(dimensionalSelection->getSelection(i));
				}
			} else {
				SharedProjectionSelection projectionSelection = selectionCast<ProjectionSelection>(selectionObj);
				if (projectionSelection) {
					SharedDimensionalSelection dSel = dimensionalSelection;
					Position origin;
					unsigned int height = 0;
					while (dSel) {
						SharedSelection sel = dSel->getSelection(0);
						SharedCellSelection cSel = selectionCast<CellSelection>(sel);
						if (cSel) {
							origin = cSel->getPosition();
							break;
						}
						height++;
						dSel = selectionCast<DimensionalSelection>(sel);
					}

					selectionsLeft.push(dimensionalSelection->getSelection(0));

					if (projectionSelection->size() == 1) {
						arrows[newElement].push_back(ArrowRenderData(origin, origin, height));
					} else {
						for (int i = 1; i < projectionSelection->size(); i++) {
							Position newOrigin = origin + projectionSelection->getStep();
							arrows[newElement].push_back(ArrowRenderData(origin, newOrigin, height));
							origin = newOrigin;
						}
					}
				} else {
					for (int i = 0; i < dimensionalSelection->size(); i++) {
						selectionsLeft.push(dimensionalSelection->getSelection(i));
					}
				}
			}
		}
	}

	return newElement;
}

ElementId ViewportRenderer::addSelectionElement(const SelectionElement& selection) {
	std::lock_guard<std::mutex> lock(elementsMux);

	ElementId newElement = ++currentElementId;

	FPosition topLeft = selection.topLeft.free();
	FPosition bottomRight = selection.bottomRight.free();
	if (selection.topLeft.x > selection.bottomRight.x) std::swap(topLeft.x, bottomRight.x);
	if (selection.topLeft.y > selection.bottomRight.y) std::swap(topLeft.y, bottomRight.y);

	BoxSelectionRenderData newBoxSelection;
	newBoxSelection.topLeft = glm::vec2(topLeft.x, topLeft.y);
	newBoxSelection.size = glm::vec2(bottomRight.x - topLeft.x + 1.0f, bottomRight.y - topLeft.y + 1.0f);
	newBoxSelection.state = selection.inverted ? BoxSelectionRenderData::Inverted : BoxSelectionRenderData::Normal;

	boxSelections[newElement].push_back(newBoxSelection);

	return newElement;
}

void ViewportRenderer::removeSelectionElement(ElementId id) {
	std::lock_guard<std::mutex> lock(elementsMux);
	boxSelections.erase(id);
	arrows.erase(id);
}

std::vector<ArrowRenderData> ViewportRenderer::getArrows() {
	std::lock_guard<std::mutex> lock(elementsMux);

	std::vector<ArrowRenderData> returnArrows;
	returnArrows.reserve(arrows.size());

	for (const auto& arrow : arrows) {
		returnArrows.insert(returnArrows.end(), arrow.second.begin(), arrow.second.end());
	}

	return returnArrows;
}

std::vector<BoxSelectionRenderData> ViewportRenderer::getBoxSelections() {
	std::lock_guard<std::mutex> lock(elementsMux);

	std::vector<BoxSelectionRenderData> returnBoxSelections;
	returnBoxSelections.reserve(boxSelections.size());

	for (const auto& boxSelection : boxSelections) {
		returnBoxSelections.insert(returnBoxSelections.end(), boxSelection.second.begin(), boxSelection.second.end());
	}

	return returnBoxSelections;
}

ElementId ViewportRenderer::addBlockPreview(BlockPreview&& blockPreview) {
	std::lock_guard<std::mutex> lock(elementsMux);

	ElementId newElement = ++currentElementId;

	blockPreviews.reserve(blockPreviews.size() + blockPreview.blocks.size());

	for (const BlockPreview::Block& block : blockPreview.blocks) {
		BlockPreviewRenderData newPreview;
		newPreview.position = glm::vec2(block.position.x, block.position.y);
		newPreview.orientation = block.orientation;
		const BlockRenderDataManager::BlockRenderData* blockRenderData = MainRenderer::get().getBlockRenderDataManager().getBlockRenderData(block.blockRenderDataId);
		if (!blockRenderData) continue;
		Size size = block.orientation * blockRenderData->size;
		newPreview.size = glm::vec2(size.w, size.h);
		newPreview.textureIndex = blockRenderData->blockTextureCords.textureLayer;
		newPreview.texPos = blockRenderData->blockTextureCords.textureOriginUV;
		newPreview.texSize = blockRenderData->blockTextureCords.textureSizeUV;

		blockPreviews.emplace(newElement, newPreview);
	}

	return newElement;
}

void ViewportRenderer::shiftBlockPreview(ElementId id, Vector shift) {
	std::lock_guard<std::mutex> lock(elementsMux);

	auto iterPair = blockPreviews.equal_range(id);
	for (auto iter = iterPair.first; iter != iterPair.second; ++iter) {
		iter->second.position += glm::vec2(shift.dx, shift.dy);
	}
}

void ViewportRenderer::removeBlockPreview(ElementId id) {
	std::lock_guard<std::mutex> lock(elementsMux);
	blockPreviews.erase(id);
}

std::vector<BlockPreviewRenderData> ViewportRenderer::getBlockPreviews() {
	std::lock_guard<std::mutex> lock(elementsMux);

	std::vector<BlockPreviewRenderData> returnBlockPreviews;
	returnBlockPreviews.reserve(blockPreviews.size());

	for (const auto& preview : blockPreviews) {
		returnBlockPreviews.push_back(preview.second);
	}

	return returnBlockPreviews;
}

ElementId ViewportRenderer::addConnectionPreview(const ConnectionPreview& connectionPreview) {
	std::lock_guard<std::mutex> lock(elementsMux);

	ElementId newElement = ++currentElementId;

	ConnectionPreviewRenderData newPreview;
	FPosition pointA = connectionPreview.output;
	FPosition pointB = connectionPreview.input;
	newPreview.pointA = glm::vec2(pointA.x, pointA.y);
	newPreview.pointB = glm::vec2(pointB.x, pointB.y);
	connectionPreviews[newElement] = newPreview;

	return newElement;
}

void ViewportRenderer::removeConnectionPreview(ElementId id) {
	std::lock_guard<std::mutex> lock(elementsMux);
	connectionPreviews.erase(id);
}

ElementId ViewportRenderer::addHalfConnectionPreview(const HalfConnectionPreview& halfConnectionPreview) {
	std::lock_guard<std::mutex> lock(elementsMux);

	ElementId newElement = ++currentElementId;

	ConnectionPreviewRenderData newPreview;
	FPosition pointA = halfConnectionPreview.output;
	newPreview.pointA = glm::vec2(pointA.x, pointA.y);
	newPreview.pointB = glm::vec2(halfConnectionPreview.input.x, halfConnectionPreview.input.y);
	connectionPreviews[newElement] = newPreview;

	return newElement;
}

void ViewportRenderer::removeHalfConnectionPreview(ElementId id) {
	std::lock_guard<std::mutex> lock(elementsMux);
	connectionPreviews.erase(id);
}

std::vector<ConnectionPreviewRenderData> ViewportRenderer::getConnectionPreviews() {
	std::lock_guard<std::mutex> lock(elementsMux);

	std::vector<ConnectionPreviewRenderData> returnConnectionPreviews;
	returnConnectionPreviews.reserve(connectionPreviews.size());

	for (const auto& preview : connectionPreviews) {
		returnConnectionPreviews.push_back(preview.second);
	}

	return returnConnectionPreviews;
}

void ViewportRenderer::render(Frame& frame) {
#ifdef TRACY_PROFILER
	ZoneScoped;
#endif

	// Get simulator and address with proper locking
	const EvalLogicSimulator* sim = getSimulator();
	Address addr = getAddress();

	// Get view data with proper locking
	ViewportViewData localViewData = getViewData();

	// Only render if we have valid data
	gridRenderer.render(frame, localViewData.viewportViewMat, localViewData.viewScale, sim);

	if (sim != nullptr) {
		chunkRenderer.render(
			frame,
			localViewData.viewportViewMat,
			sim,
			addr,
			getChunker().getAllocations(localViewData.viewBounds.first.snap(), localViewData.viewBounds.second.snap())
		);
	}

	elementRenderer.renderBlockPreviews(frame, localViewData.viewportViewMat, getBlockPreviews());
	elementRenderer.renderConnectionPreviews(frame, localViewData.viewportViewMat, getConnectionPreviews());
	elementRenderer.renderBoxSelections(frame, localViewData.viewportViewMat, getBoxSelections());
	elementRenderer.renderArrows(frame, localViewData.viewportViewMat, getArrows());
}

void ViewportRenderer::createRenderPass() {
	VkSampleCountFlagBits msaaSamples = device->getMaxUsableSampleCount();

	// MSAA color attachment (we render to this)
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	colorAttachment.samples = msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Don't need to store MSAA
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Resolve attachment (non-MSAA, for ImGui to read)
	VkAttachmentDescription resolveAttachment{};
	resolveAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear on each frame
	resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // We need this for ImGui
	resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Don't care about previous contents
	resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Ready for ImGui sampling

	// Attachment references
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference resolveAttachmentRef{};
	resolveAttachmentRef.attachment = 1;
	resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Subpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pResolveAttachments = &resolveAttachmentRef;

	// Subpass dependency for layout transitions
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Create render pass with both attachments
	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, resolveAttachment};
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device->getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create MSAA render pass!");
	}
}

void ViewportRenderer::renderLoop() {
	while(running) {
		if (recreateImages) {
			{
				std::lock_guard<std::mutex> lock(viewMux);
				viewData = newViewData;
			}

			// Wait for device idle before recreating images
			device->waitIdle();

			// Mark images as not ready
			imagesReady = false;

			{
				std::lock_guard<std::mutex> lock(imageMux);
				// Clear the read texture while we're recreating
				imguiReadTexture = VK_NULL_HANDLE;
			}

			destroyImages();
			createImages();

			// Mark images as ready only after creation succeeds
			imagesReady = true;
			recreateImages = false;
		}

		// Don't try to render if images aren't ready
		if (!imagesReady || msaaImages.empty() || resolveImages.empty() || imguiTextures.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
			continue;
		}

		Frame& frame = frames.getCurrentFrame();

		// Wait for this frame to complete
		frames.waitForCurrentFrameCompletion();

		// Mark frame as started
		frames.startCurrentFrame();

		// Get the current render target index
		uint32_t renderIndex = frames.getCurrentFrameIndex();

		// Record command buffer
		vkResetCommandBuffer(frame.mainCommandBuffer, 0);
		renderToCommandBuffer(frame, renderIndex);

		// Submit to graphics queue
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &frame.mainCommandBuffer;

		if (device->submitGraphicsQueue(&submitInfo, frame.renderFence) != VK_SUCCESS) {
			// Handle error - submission failed
			continue;
		}

		// Update which texture ImGui should read from (only after successful submission)
		{
			std::lock_guard<std::mutex> lock(imageMux);
			imguiReadTexture = imguiTextures[renderIndex];
			// Mark images as ready after first successful render
			if (!imagesReady) {
				imagesReady = true;
			}
		}

		frames.incrementFrame();
	}
}

void ViewportRenderer::renderToCommandBuffer(Frame& frame, uint32_t imageIndex) {
	// Start recording
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(frame.mainCommandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	// Clear color
	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } }; // MSAA attachment
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } }; // Resolve attachment

	// Get view data once for the whole render
	ViewportViewData localViewData;
	{
		std::lock_guard<std::mutex> lock(viewMux);
		localViewData = viewData;
	}

	// Begin render pass
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = localViewData.viewportSize;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(frame.mainCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Set dynamic viewport and scissor
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(localViewData.viewportSize.width);
	viewport.height = static_cast<float>(localViewData.viewportSize.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(frame.mainCommandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = localViewData.viewportSize;
	vkCmdSetScissor(frame.mainCommandBuffer, 0, 1, &scissor);

	// Render viewport content - only if viewport has valid dimensions
	if (localViewData.viewportSize.width > 0 && localViewData.viewportSize.height > 0) {
		render(frame);
	}

	// End render pass (automatically resolves MSAA to resolve image)
	vkCmdEndRenderPass(frame.mainCommandBuffer);

	if (vkEndCommandBuffer(frame.mainCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void ViewportRenderer::destroyImages() {
	// Mark images as not ready immediately
	imagesReady = false;

	// Wait for all frames to complete their rendering
	// This ensures ImGui is done using the descriptor sets
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		frames.waitForCurrentFrameCompletion();
		frames.incrementFrame();
	}

	// Also wait for device to be completely idle
	device->waitIdle();

	// Clear the read texture so new ImGui frames won't try to use it
	imguiReadTexture = VK_NULL_HANDLE;

	// Now safe to destroy ImGui textures
	for (VkDescriptorSet texture : imguiTextures) {
		if (texture != VK_NULL_HANDLE) {
			ImGui_ImplVulkan_RemoveTexture(texture);
		}
	}
	imguiTextures.clear();

	// Destroy framebuffers
	for (VkFramebuffer framebuffer : framebuffers) {
		vkDestroyFramebuffer(device->getDevice(), framebuffer, nullptr);
	}
	framebuffers.clear();

	// Destroy images
	for (AllocatedImage& image : msaaImages) {
		destroyImage(image);
	}
	msaaImages.clear();

	for (AllocatedImage& image : resolveImages) {
		destroyImage(image);
	}
	resolveImages.clear();
}

void ViewportRenderer::createImages() {
	if (viewData.viewportSize.width == 0 || viewData.viewportSize.height == 0) {
		return;
	}

	VkExtent3D imageSize = {viewData.viewportSize.width, viewData.viewportSize.height, 1};
	VkSampleCountFlagBits msaaSamples = device->getMaxUsableSampleCount();

	// Resize arrays
	msaaImages.resize(FRAMES_IN_FLIGHT);
	resolveImages.resize(FRAMES_IN_FLIGHT);
	framebuffers.resize(FRAMES_IN_FLIGHT);
	imguiTextures.resize(FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		// Create MSAA image (transient - doesn't need to be stored)
		msaaImages[i] = createImage(
			device,
			imageSize,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			false,
			msaaSamples
		);

		// Create resolve image (non-MSAA, for ImGui to sample)
		resolveImages[i] = createImage(
			device,
			imageSize,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			false,
			VK_SAMPLE_COUNT_1_BIT
		);

		// Create framebuffer with both attachments
		std::array<VkImageView, 2> attachments = {
			msaaImages[i].imageView,    // Color attachment (MSAA)
			resolveImages[i].imageView  // Resolve attachment (non-MSAA)
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = viewData.viewportSize.width;
		framebufferInfo.height = viewData.viewportSize.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device->getDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}

		// Create ImGui descriptor for the resolve image
		imguiTextures[i] = ImGui_ImplVulkan_AddTexture(
			sampler,
			resolveImages[i].imageView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	// Don't set imguiReadTexture here - let the render loop set it after the first frame renders
	// This ensures we never expose an image that hasn't been rendered to yet
}
