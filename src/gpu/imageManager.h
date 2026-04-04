#ifndef imageManager_h
#define imageManager_h

#include "gpu/abstractions/vulkanImage.h"
#include "stb_image.h"

class ImageManager {
public:
	ImageManager(VulkanDevice& device) : device(device) {}

	const AllocatedImage* getImage(std::string path) {
		int width, height, channels;
		stbi_uc* pixels = stbi_load("my_image.png", &width, &height, &channels, STBI_rgb_alpha);
		if (!pixels) return nullptr;
		// VulkanDevice& device, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT
		auto pair = loadedImages.emplace(path, pixels, device, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		return pair.first.second;
	}

private:
	VulkanDevice& device;
	std::mutex loadedImagesMux;
	std::unordered_map<std::string, AllocatedImage> loadedImages
}

#endif /* imageManager_h */
