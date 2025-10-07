#include "blockTextureManager.h"

#include "computerAPI/directoryManager.h"
#include "gpu/vulkanDevice.h"

#include <stb_image.h>

BlockTexture::~BlockTexture() {
	destroyImage(image);
	vkDestroySampler(device->getDevice(), sampler, nullptr);
}

void BlockTextureManager::init(VulkanDevice* device) {
        this->device = device;

        textureArray = std::make_shared<BlockTextureArray>();
        textureArray->device = device;
        textureArray->maxLayers = 5; // TODO
        textureArray->nextFreeLayer = 0;
		textureArray->image = createImageArray(
			device,
			extent,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			layers
		);

		addTexture("logicTiles.png");

        // Create sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vkCreateSampler(device->getDevice(), &samplerInfo, nullptr, &textureArray->sampler);

        // Create descriptor set
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        descriptorLayout = device->createDescriptorSetLayout({binding});
        textureArray->descriptor = device->allocateDescriptorSet(descriptorLayout);

        VkDescriptorImageInfo imageInfoDS{};
        imageInfoDS.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfoDS.imageView = textureArray->image.view;
        imageInfoDS.sampler = textureArray->sampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = textureArray->descriptor;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imageInfoDS;
        vkUpdateDescriptorSets(device->getDevice(), 1, &write, 0, nullptr);
}

void copyTextureToLayer(VulkanDevice* device, uint8_t* pixels, int width, int height, VkImage image, uint32_t layer) {
    VkDeviceSize imageSize = width * height * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    device->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(device->getDevice(), stagingMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device->getDevice(), stagingMemory);

    VkCommandBuffer cmd = beginSingleTimeCommands(device);

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = layer;
    range.layerCount = 1;

    transitionImageLayout(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    transitionImageLayout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);

    endSingleTimeCommands(device, cmd);

    vkDestroyBuffer(device->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(device->getDevice(), stagingMemory, nullptr);
}

uint32_t BlockTextureManager::addTexture(const std::string& path) {
	if (textureArray->nextFreeLayer >= textureArray->maxLayers) {
        throw std::runtime_error("Texture array is full!");
    }

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("Failed to load texture: " + filepath);
    }

    VkDeviceSize imageSize = texWidth * texHeight * 4;

    // --- Create staging buffer ---
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    textureArray->device->createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory
    );

    void* data;
    vkMapMemory(textureArray->device->logicalDevice, stagingMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(textureArray->device->logicalDevice, stagingMemory);

    stbi_image_free(pixels);

    // --- Copy buffer → image layer ---
    VkCommandBuffer cmd = textureArray->device->beginSingleTimeCommands();

	transitionImageLayout(cmd, textureArray->image,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = textureArray->nextFreeLayer;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight),
        1
    };

    vkCmdCopyBufferToImage(cmd, stagingBuffer, textureArray->image.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    transitionImageLayout(cmd, textureArray->image,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    textureArray->device->endSingleTimeCommands(cmd);

    vkDestroyBuffer(textureArray->device->logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(textureArray->device->logicalDevice, stagingMemory, nullptr);

    textureArray->nextFreeLayer++;
}

void BlockTextureManager::cleanup() {

	// destroy image
	vkDestroyDescriptorSetLayout(device->getDevice(), descriptorLayout, nullptr);

	textureArray.reset();

	// destroy descriptor allocator
	descriptorAllocator.cleanup();
}
