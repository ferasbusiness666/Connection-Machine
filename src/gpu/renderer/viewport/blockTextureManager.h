#ifndef blockTextureManager_h
#define blockTextureManager_h

#include "gpu/abstractions/vulkanDescriptor.h"
#include "gpu/abstractions/vulkanImage.h"
#include "tileSet.h"

class VulkanDevice;

struct BlockTexture {
	~BlockTexture();
	VkDescriptorSet descriptor;
	VkSampler sampler;
	AllocatedImage image;
	uint32_t layer;
	VulkanDevice* device;
};

struct BlockTextureArray {
    VulkanDevice* device;
    AllocatedImage image;
    VkSampler sampler;
    VkDescriptorSet descriptor;

    uint32_t maxLayers;
    uint32_t nextFreeLayer;

    BlockTextureArray() : device(nullptr), sampler(VK_NULL_HANDLE), descriptor(VK_NULL_HANDLE),
                          maxLayers(0), nextFreeLayer(0) {}
	~BlockTextureArray();
};

class BlockTextureManager {
public:
	void init(VulkanDevice* device);
	void addTexture(const std::string& path);
	void update();
	void cleanup();

	inline VkDescriptorSetLayout getDescriptorLayout() { return descriptorLayout; }
    inline std::shared_ptr<BlockTextureArray> getTextureArray() { return textureArray; }

private:
	VulkanDevice* device;

	DescriptorAllocator descriptorAllocator;
	VkDescriptorSetLayout descriptorLayout;

	std::shared_ptr<BlockTextureArray> textureArray;
};

#endif
