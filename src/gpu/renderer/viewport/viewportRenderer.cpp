#include "viewportRenderer.h"

#include <glm/ext/matrix_clip_space.hpp>

#include "backend/selection.h"
#include "gpu/abstractions/vulkanImage.h"
#include "gpu/mainRenderer.h"
#include "gpu/renderer/viewport/elements/elementRenderer.h"
#include "imgui/imgui_impl_vulkan.h"

#ifdef TRACY_PROFILER
#include <tracy/Tracy.hpp>
#endif

ViewportRenderer::Sampler::Sampler(VkSampler sampler) : sampler(sampler) { }
ViewportRenderer::Sampler::~Sampler() { vkDestroySampler(MainRenderer::get().getVulkanInstance().getDevice()->getDevice(), sampler, nullptr); }

ViewportRenderer::ViewportRenderer(VulkanDevice* device, ImGuiRenderer& imGuiRenderer) : imGuiRenderer(imGuiRenderer), chunker(device), device(device) {
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
	VkSampler vkSampler;
	vkCreateSampler(device->getDevice(), &samplerInfo, nullptr, &vkSampler);
	sampler = std::make_shared<Sampler>(vkSampler);

	frames.init(device);

	imageSwapchain.init(device);
	imguiTextures.resize(FRAMES_IN_FLIGHT);
}

ViewportRenderer::~ViewportRenderer() {
	{
		std::lock_guard guard(MainRenderer::get().getVulkanInstance().getDevice()->getGraphicsQueueLock());
		destroyImages();
	}

	imageSwapchain.cleanup();

	vkDestroyRenderPass(device->getDevice(), renderPass, nullptr);

	elementRenderer.cleanup();
	chunkRenderer.cleanup();
	gridRenderer.cleanup();
	frames.cleanup();
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
	{
		std::lock_guard<std::mutex> lock(viewMux);
		newViewData.viewportSize.width = size.x;
		newViewData.viewportSize.height = size.y;
	}
	if (viewData.viewportSize.width != newViewData.viewportSize.width || viewData.viewportSize.height != newViewData.viewportSize.height) {
		updateViewData.store(true);
	}
}

void ViewportRenderer::updateView(FPosition topLeft, FPosition bottomRight) {
	{
		std::lock_guard<std::mutex> lock(viewMux);
		newViewData.viewportViewMat = glm::ortho(topLeft.x, bottomRight.x, topLeft.y, bottomRight.y);
		newViewData.viewBounds = { topLeft, bottomRight };
		float viewWidth = bottomRight.x - topLeft.x;
		float viewHeight = bottomRight.y - topLeft.y;
		newViewData.viewScale = std::min(viewWidth, viewHeight);
	}
	updateViewData.store(true);
}

class BorrowedImageDetector {
public:
	BorrowedImageDetector(std::atomic<int>& currentBorrowedImage) : currentBorrowedImage(currentBorrowedImage) { }
	~BorrowedImageDetector() { currentBorrowedImage.store(-1); }
private:
	std::atomic<int>& currentBorrowedImage;
};

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
	resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	  // Clear on each frame
	resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // We need this for ImGui
	resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			  // Don't care about previous contents
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
	std::array<VkSubpassDependency, 2> dependencies{};

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	// Create render pass with both attachments
	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, resolveAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device->getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create MSAA render pass!");
	}
}

#ifdef TRACY_PROFILER
const char* const viewportLoop_Tracy = "ViewportLoop";
#endif

// std::pair<VkDescriptorSet, std::shared_ptr<void>> ViewportRenderer::getLatestImage() {
// 	std::lock_guard<std::mutex> lock(imageMux);
// 	if (!imageReady || imguiTextures.size() < FRAMES_IN_FLIGHT || imagesReady.load() < FRAMES_IN_FLIGHT) {
// 		return { VK_NULL_HANDLE, nullptr };
// 	}
// 	{
// 		unsigned int lastFrameIndex = (frames.getCurrentFrameIndex() - 1) % FRAMES_IN_FLIGHT;
// 		VkDescriptorSet descriptorSet = imguiTextures[lastFrameIndex];
// 		if (descriptorSet == VK_NULL_HANDLE) return { VK_NULL_HANDLE, nullptr };
// 		currentBorrowedImage.store(lastFrameIndex);
// 		return { descriptorSet, std::make_shared<BorrowedImageDetector>(currentBorrowedImage) } ;
// 	}
// }

std::tuple<VkDescriptorSet, VkSemaphore, std::vector<std::shared_ptr<void>>> ViewportRenderer::startImageRender() {
	// while(running.load()) {
	// 	if (currentBorrowedImage.load() != -1) {
	// 		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	// 		continue;
	// 	}
	uint32_t currentFrameIndex = frames.getCurrentFrameIndex();

	if (updateViewData.load()) {
		bool recreateImages = false;
		{
			if (viewData.viewportSize.width == 0 || viewData.viewportSize.height == 0) {
				viewData.viewportSize.width = 1;
				viewData.viewportSize.height = 1;
				recreateImages = true;
			}
			std::lock_guard<std::mutex> lock(viewMux);
			if (newViewData.viewportSize.width == 0 || newViewData.viewportSize.height == 0) {
				newViewData.viewportSize.width = 1;
				newViewData.viewportSize.height = 1;
			}
			if (viewData.viewportSize.width != newViewData.viewportSize.width || viewData.viewportSize.height != newViewData.viewportSize.height) {
				recreateImages = true;
			}
			updateViewData.store(false);
			viewData = newViewData;
		}
		if (recreateImages) {
			imagesReady.store(0);
			std::lock_guard guard(MainRenderer::get().getVulkanInstance().getDevice()->getGraphicsQueueLock());
			destroyImages();
			createImages();
		}
	} else if (imguiTextures.empty()) {
		destroyImages();
		createImages();
	}

	// Wait for this frame to complete
	frames.waitForCurrentFrameCompletion();
	// #ifdef TRACY_PROFILER
	// FrameMarkEnd(viewportLoop_Tracy);
	// #endif

	// std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
	// std::chrono::nanoseconds deltaTime = now - lastUpdateRender;
	// // force 60 fps
	// if (deltaTime.count() < 16 * 1000000) {
	// 	std::this_thread::sleep_for(std::chrono::nanoseconds(16 * 1000000 - deltaTime.count()));
	// }
	// // get real time between frames
	// now = std::chrono::high_resolution_clock::now();
	// deltaTime = now - lastUpdateRender;
	// lastUpdateRender = now;
	// float alpha = 0.2f;
	// fps.store((alpha * 1000000000. / (float)deltaTime.count()) + (1.0 - alpha) * fps);

	std::shared_ptr<Frame> frame = frames.getCurrentFrame();

	// Mark frame as started
	// #ifdef TRACY_PROFILER
	// FrameMarkStart(viewportLoop_Tracy);
	// #endif
	frames.startCurrentFrame();

	// Record command buffer
	{
		std::lock_guard guard(MainRenderer::get().getVulkanInstance().getDevice()->getGraphicsQueueLock());
		vkResetCommandBuffer(frame->mainCommandBuffer, 0);
	}
	renderToCommandBuffer(*frame, currentFrameIndex);

	std::shared_ptr<ImageSwapchain::Semaphore> semaphore = imageSwapchain.getImageSemaphores()[currentFrameIndex];

	// Submit to graphics queue
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &semaphore->semaphore;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame->mainCommandBuffer;

	if (device->submitGraphicsQueue(&submitInfo, frame->renderFence) != VK_SUCCESS) {
		// Handle error - submission failed
		return { VK_NULL_HANDLE, VK_NULL_HANDLE, {} };
	}

	{
		std::lock_guard<std::mutex> lock(imageMux);
		imageReady.store(true);
	}
	{
		std::lock_guard<std::mutex> lock(framesMutex);
		frames.incrementFrame();
		imagesReady.fetch_add(1);
	}
	// }
	// device->waitIdle();
	return { imguiTextures[currentFrameIndex]->descriptorSet, semaphore->semaphore, { frame, imguiTextures[currentFrameIndex], semaphore } };
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

	// Begin render pass
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = imageSwapchain.getFramebuffers()[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = viewData.viewportSize;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(frame.mainCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Set dynamic viewport and scissor
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(viewData.viewportSize.width);
	viewport.height = static_cast<float>(viewData.viewportSize.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(frame.mainCommandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = viewData.viewportSize;
	vkCmdSetScissor(frame.mainCommandBuffer, 0, 1, &scissor);

	// Render viewport content - only if viewport has valid dimensions
	if (viewData.viewportSize.width > 0 && viewData.viewportSize.height > 0) {
		render(frame);
	}

	// End render pass (automatically resolves MSAA to resolve image)
	vkCmdEndRenderPass(frame.mainCommandBuffer);

	if (vkEndCommandBuffer(frame.mainCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void ViewportRenderer::destroyImages() {
	if (imageReady.load()) {
		device->waitIdleNoMux();
		imageReady.store(false);
		imguiTextures.clear(); // will clear when done
		msaaImage.reset();
	}
}

void ViewportRenderer::createImages() {
	VkExtent3D imageSize = { viewData.viewportSize.width, viewData.viewportSize.height, 1 };
	VkSampleCountFlagBits msaaSamples = device->getMaxUsableSampleCount();

	// Create MSAA image (transient - doesn't need to be stored)
	msaaImage.emplace(device, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, false, msaaSamples);

	imageSwapchain.recreate(renderPass, { viewData.viewportSize.width, viewData.viewportSize.height }, *msaaImage);
	imguiTextures.resize(FRAMES_IN_FLIGHT);
	for (unsigned int i = 0; i < imguiTextures.size(); i++) {
		// std::lock_guard lock(imGuiRenderer.setActiveContext()); // not needed this is only called from the render func of a widget
		imguiTextures[i] = std::make_shared<ImGuiRenderer::ImGuiDescriptorSet>(
			ImGui_ImplVulkan_AddTexture(sampler->sampler, imageSwapchain.getImages()[i]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
			imGuiRenderer,
			std::vector<std::shared_ptr<void>>{ imageSwapchain.getImages()[i], sampler }
		);
	}

	imageReady.store(true);
}
