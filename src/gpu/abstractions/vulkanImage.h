#ifndef vulkanImage_h
#define vulkanImage_h

#include <vk_mem_alloc.h>

class VulkanDevice;

struct AllocatedImage {
	AllocatedImage(VulkanDevice& device, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false, uint32_t arrayLayers = 1, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
	AllocatedImage(VulkanDevice& device, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
	AllocatedImage(VulkanDevice& device, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
	~AllocatedImage();
    VkImage image;
    VkImageView imageView;
	std::vector<VkImageView> layerViews;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
	VkImageAspectFlags aspect;
	uint32_t mipLevels;
	uint32_t arrayLayers;

	VulkanDevice& device;
};

bool transitionImageLayout(VkCommandBuffer cmd, AllocatedImage& image, VkImageLayout oldLayout, VkImageLayout newLayout);

#endif /* vulkanImage_h */
