#ifndef blockTextureManager_h
#define blockTextureManager_h

#include "glm/ext/vector_float2.hpp"
#include "gpu/abstractions/vulkanDescriptor.h"
#include "gpu/abstractions/vulkanImage.h"
#include "util/vec2.h"

class VulkanDevice;

#include <stb_image.h>

struct BlockTextureArray {
    VulkanDevice* device;
    AllocatedImage image;
    VkSampler sampler;
    VkDescriptorSet descriptor;
	VkExtent3D texSize;

    uint32_t maxLayers;
    uint32_t nextFreeLayer;

    BlockTextureArray() : device(nullptr), sampler(VK_NULL_HANDLE), descriptor(VK_NULL_HANDLE),
                          maxLayers(0), nextFreeLayer(0) {}
	~BlockTextureArray();
};

typedef unsigned int BlockTextureId;

struct BlockTexture {
public:
	BlockTexture(glm::vec2 textureOrigin, glm::vec2 texSize, unsigned int texLayer) : textureOrigin(textureOrigin), texSize(texSize), texLayer(texLayer) {}
	glm::vec2 textureOrigin;
	glm::vec2 texSize;
	unsigned int texLayer;
};

struct BlockTextureCords {
public:
	BlockTextureCords(glm::vec2 textureOriginUV, glm::vec2 texSizeUV, unsigned int texLayer) : textureOriginUV(textureOriginUV), texSizeUV(texSizeUV), texLayer(texLayer) {}
	bool operator==(const BlockTextureCords& other) const {
		return textureOriginUV == other.textureOriginUV && texSizeUV == other.texSizeUV && texLayer == other.texLayer;
	}
	glm::vec2 textureOriginUV;
	glm::vec2 texSizeUV;
	unsigned int texLayer;
};

class BlockTextureManager {
public:
	void init(VulkanDevice* device);
	BlockTextureId addTexture(const std::string& path);
	void refreshBlockTexture(const std::string& path);
	BlockTextureCords getBlockTextureCords(BlockTextureId blockTextureId) const;
	BlockTextureCords getBlockTextureCords(BlockTextureId blockTextureId, Vec2Int tileSize, Vec2Int smallestCordTile, Vec2Int blockSize) const;
	void update();
	void cleanup();

	inline VkDescriptorSetLayout getDescriptorLayout() { return descriptorLayout; }
    inline std::shared_ptr<BlockTextureArray> getTextureArray() { return textureArray; }

private:
	// this needs to free pixels
	std::pair<glm::vec2, uint32_t> addTextureToArray(stbi_uc* pixels, int texWidth, int texHeight);

	VulkanDevice* device;

	DescriptorAllocator descriptorAllocator;
	VkDescriptorSetLayout descriptorLayout;

	std::shared_ptr<BlockTextureArray> textureArray;
	std::map<BlockTextureId, BlockTexture> blockTextures;
	std::map<std::string, BlockTextureId> loadedTextureFiles;
};

#endif
