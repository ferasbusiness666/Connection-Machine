#ifndef blockTextureManager_h
#define blockTextureManager_h

#include "gpu/abstractions/vulkanDescriptor.h"
#include "gpu/abstractions/vulkanImage.h"
#include "tileSet.h"

class VulkanDevice;

struct BlockTextureArray {
    VulkanDevice* device;
    AllocatedImage image;
    VkSampler sampler;
    VkDescriptorSet descriptor;

    uint32_t maxLayers;
    uint32_t nextFreeLayer;

    BlockTextureArray() : device(nullptr), sampler(VK_NULL_HANDLE), descriptor(VK_NULL_HANDLE),
                          maxLayers(0), nextFreeLayer(0) {}
};

class BlockTextureManager {
public:
	void init(VulkanDevice* device);
	uint32_t addTexture(const std::string& path);
	void update();
	void cleanup();

	inline VkDescriptorSetLayout getDescriptorLayout() { return descriptorLayout; }
	inline TileSetInfo& getTileset() { return mainTileSet; }

private:
	VulkanDevice* device;

	DescriptorAllocator descriptorAllocator;
	VkDescriptorSetLayout descriptorLayout;

	std::shared_ptr<BlockTextureArray> textureArray;
	TileSetInfo mainTileSet = TileSetInfo(256, 15, 4);
};

#endif
