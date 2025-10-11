#include "blockTextureManager.h"

#include "gpu/vulkanDevice.h"
#include "util/algorithm.h"

BlockTextureArray::~BlockTextureArray() {
	destroyImage(image);
	vkDestroySampler(device->getDevice(), sampler, nullptr);
}

void BlockTextureManager::init(VulkanDevice* device) {
	this->device = device;

	descriptorAllocator.init(device, 100, { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0f } });

	VkExtent3D texSize = { 4096, 4096, 1 };
	textureArray = std::make_shared<BlockTextureArray>();
	textureArray->texSize = texSize;
	textureArray->device = device;
	textureArray->maxLayers = 1;
	textureArray->nextFreeLayer = 0;
	textureArray->image = createImage(device, texSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, false, textureArray->maxLayers);

	// create layout and descriptor set
	DescriptorLayoutBuilder textureLayoutBuilder;
	textureLayoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorLayout = textureLayoutBuilder.build(device->getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT);
	textureArray->descriptor = descriptorAllocator.allocate(descriptorLayout);

	// create sampler
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	vkCreateSampler(device->getDevice(), &samplerInfo, nullptr, &textureArray->sampler);

	// write descriptor
	DescriptorWriter textureWriter;
	textureWriter.writeImage(0, textureArray->image.imageView, textureArray->sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	textureWriter.updateSet(device->getDevice(), textureArray->descriptor);
}

void BlockTextureManager::resizeTextureArray(uint32_t newLayerCount) {
	VulkanDevice* dev = textureArray->device;
	auto oldImage = textureArray->image;
	uint32_t oldLayerCount = textureArray->maxLayers;

	AllocatedImage newImage = createImage(
		dev,
		textureArray->texSize,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		false,
		newLayerCount
	);

	dev->immediateSubmit([&](VkCommandBuffer cmd) {
		for (uint32_t layer = 0; layer < oldLayerCount; layer++) {
			VkImageCopy region{};
			region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, layer, 1 };
			region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, layer, 1 };
			region.extent = { textureArray->texSize.width, textureArray->texSize.height, 1 };
			vkCmdCopyImage(cmd,
				oldImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &region);
		}
	});

	destroyImage(oldImage);
	textureArray->image = newImage;
	textureArray->maxLayers = newLayerCount;

	DescriptorWriter writer;
	writer.writeImage(0, newImage.imageView, textureArray->sampler,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.updateSet(dev->getDevice(), textureArray->descriptor);
}

BlockTextureId BlockTextureManager::addTexture(const std::string& path) {
	// check if its already loaded
	auto iter = loadedTextureFiles.find(path);
	if (iter != loadedTextureFiles.end()) {
		return iter->second;
	}

	// --- Load pixels from file ---
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		throwFatalError("Failed to load texture: " + path);
	}

	std::pair<glm::vec2, uint32_t> pair = addTextureToArray(pixels, texWidth, texHeight);

	BlockTextureId blockTextureId = findUnusedKey<BlockTextureId>(blockTextures, 1);
	blockTextures.try_emplace(blockTextureId, pair.first, glm::vec2(texWidth, texHeight), pair.second);

	loadedTextureFiles.emplace(path, blockTextureId);

	return blockTextureId;
}

BlockTextureId BlockTextureManager::refreshBlockTexture(const std::string& path) {
	// check if its already loaded
	auto iter = loadedTextureFiles.find(path);
	if (iter == loadedTextureFiles.end()) {
		return iter->second;
	}

	// --- Load pixels from file ---
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		throwFatalError("Failed to load texture: " + path);
	}

	std::pair<glm::vec2, uint32_t> pair = addTextureToArray(pixels, texWidth, texHeight);

	BlockTextureId blockTextureId = findUnusedKey<BlockTextureId>(blockTextures, 1);
	blockTextures.try_emplace(blockTextureId, pair.first, glm::vec2(texWidth, texHeight), pair.second);

	loadedTextureFiles.emplace(path, blockTextureId);

	return blockTextureId;
}

BlockTextureCords BlockTextureManager::getBlockTextureCords(BlockTextureId blockTextureId) const {
	auto blockTextureIter = blockTextures.find(blockTextureId);
	if (blockTextureIter == blockTextures.end()) {
		logError("Could not find block texture with id {}", "BlockTextureManager", blockTextureId);
		return { glm::vec2(0, 0), glm::vec2(0, 0), 0 };
	}
	return BlockTextureCords(
		glm::vec2(
			(double)(blockTextureIter->second.textureOrigin.x) / (double)textureArray->texSize.width,
			(double)(blockTextureIter->second.textureOrigin.y) / (double)textureArray->texSize.height
		),
		glm::vec2((double)(blockTextureIter->second.texSize.x) / (double)textureArray->texSize.width, (double)(blockTextureIter->second.texSize.y) / (double)textureArray->texSize.height),
		blockTextureIter->second.texLayer
	);
}

BlockTextureCords BlockTextureManager::getBlockTextureCords(BlockTextureId blockTextureId, Vec2Int tileSize, Vec2Int smallestCordTile, Vec2Int blockSize) const {
	auto blockTextureIter = blockTextures.find(blockTextureId);
	if (blockTextureIter == blockTextures.end()) {
		logError("Could not find block texture with id {}", "BlockTextureManager", blockTextureId);
		return { glm::vec2(0, 0), glm::vec2(0, 0), 0 };
	}
	return BlockTextureCords(
		glm::vec2(
			(double)(blockTextureIter->second.textureOrigin.x + smallestCordTile.x * tileSize.x) / (double)textureArray->texSize.width,
			(double)(blockTextureIter->second.textureOrigin.y + smallestCordTile.y * tileSize.y) / (double)textureArray->texSize.height
		),
		glm::vec2((double)(blockSize.x * tileSize.x) / (double)textureArray->texSize.width, (double)(blockSize.y * tileSize.y) / (double)(textureArray->texSize.height)),
		blockTextureIter->second.texLayer
	);
}

void BlockTextureManager::cleanup() {
	// destroy image
	vkDestroyDescriptorSetLayout(device->getDevice(), descriptorLayout, nullptr);

	textureArray.reset();

	// destroy descriptor allocator
	descriptorAllocator.cleanup();
}

std::pair<glm::vec2, uint32_t> BlockTextureManager::addTextureToArray(stbi_uc* pixels, int texWidth, int texHeight) {
	if (textureArray->nextFreeLayer >= textureArray->maxLayers) {
		resizeTextureArray(textureArray->maxLayers + 1);
	}

	VkDevice device = textureArray->device->getDevice();

	VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) * texHeight * 4;

	// --- Create staging buffer ---
	AllocatedBuffer stagingBuffer = createBuffer(textureArray->device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

	// Copy pixels into the staging buffer
	vmaCopyMemoryToAllocation(textureArray->device->getAllocator(), pixels, stagingBuffer.allocation, 0, imageSize);

	stbi_image_free(pixels);

	// --- Upload to the array image layer ---
	textureArray->device->immediateSubmit([&](VkCommandBuffer cmd) {
		// Transition target layer to TRANSFER_DST_OPTIMAL
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = textureArray->image.image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = textureArray->nextFreeLayer;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		// Copy staging buffer → texture layer
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = textureArray->nextFreeLayer;
		region.imageSubresource.layerCount = 1;

		region.imageExtent = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };

		vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, textureArray->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		// Transition layer to SHADER_READ_ONLY_OPTIMAL
		VkImageMemoryBarrier shaderBarrier = barrier;
		shaderBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		shaderBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		shaderBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		shaderBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &shaderBarrier);
	});

	destroyBuffer(stagingBuffer);

	return {{0, 0}, (textureArray->nextFreeLayer)++}; // needs to be post inc
}
