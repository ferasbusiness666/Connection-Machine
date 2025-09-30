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

    descriptorAllocator.init(device, 100, { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0f } });

    uint32_t maxLayers = 5; // TODO: choose a cap

    textureArray = std::make_shared<BlockTextureArray>();
    textureArray->device = device;
	textureArray->nextFreeLayer = 0;
    textureArray->maxLayers = maxLayers;

	uint32_t atlasLayer = addTexture("logicTiles.png");

    // make sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    vkCreateSampler(device->getDevice(), &samplerInfo, nullptr, &textureArray->sampler);

    // make descriptor
    DescriptorLayoutBuilder textureLayoutBuilder;
    textureLayoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descriptorLayout = textureLayoutBuilder.build(device->getDevice(), VK_SHADER_STAGE_FRAGMENT_BIT);
    textureArray->descriptor = descriptorAllocator.allocate(descriptorLayout);

    DescriptorWriter writer;
    writer.writeImage(0, textureArray->image.imageView, textureArray->sampler,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(device->getDevice(), textureArray->descriptor);
}

uint32_t BlockTextureManager::addTexture(const std::string& path) {
    // load pixels
    int texWidth = 0, texHeight = 0, texChannels = 0;

	stbi_uc* pixels = stbi_load((DirectoryManager::getResourceDirectory() / path).generic_string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		throwFatalError("Failed to load texture: " + path + " — reason: " + std::string(stbi_failure_reason()));
	}

    VkExtent3D layerSize{ (uint32_t)texWidth, (uint32_t)texHeight, 1 };
    textureArray->image = createImage(
        device,
        layerSize,                                           // <- this makes it call the VkExtent3D overload
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        false,                                               // mipmapped? false for now
        textureArray->maxLayers
    );

	std::cout << "Image handle: " << textureArray->image.image << std::endl;
	if (textureArray->image.image == VK_NULL_HANDLE) {
		throwFatalError("createImage returned null image handle");
	}

    // allocate layer
    uint32_t layer = textureArray->nextFreeLayer++;

    // compute image size (assume RGBA8)
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) * static_cast<VkDeviceSize>(texHeight) * 4ull;

    // Create staging buffer (mapped)
    AllocatedBuffer staging = createBuffer(
        device,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
    );

    // copy into mapped staging memory
    if (staging.info.pMappedData) {
        memcpy(staging.info.pMappedData, pixels, static_cast<size_t>(imageSize));
        // if you need to flush, your createBuffer + flags probably made it coherent; otherwise use vmaFlushAllocation if you have it
    }
    stbi_image_free(pixels);

    VulkanDevice& vkDev = *device;

    vkDev.immediateSubmit([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // or SHADER_READ if previously used; UNDEFINED fine for first upload
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = textureArray->image.image; // AllocatedImage.image -> raw VkImage
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = layer;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        // copy region: buffer -> image[layer]
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = layer;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };

        vkCmdCopyBufferToImage(
            cmd,
            staging.buffer,
            textureArray->image.image, // raw VkImage
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        // transition the layer to SHADER_READ_ONLY_OPTIMAL
        VkImageMemoryBarrier barrier2 = barrier;
        barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier2
        );
    });

    // cleanup staging buffer
    destroyBuffer(staging);

    return layer;
}

void BlockTextureManager::cleanup() {

	// destroy image
	vkDestroyDescriptorSetLayout(device->getDevice(), descriptorLayout, nullptr);

	mainTexture.reset();

	// destroy descriptor allocator
	descriptorAllocator.cleanup();
}
