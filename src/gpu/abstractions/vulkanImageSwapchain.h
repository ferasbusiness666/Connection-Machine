#ifndef vulkanSwapchain_h
#define vulkanSwapchain_h

#include "gpu/vulkanDevice.h"

class ImageSwapchain {
public:
	void init(VulkanDevice* device);
	void cleanup();

	void create(VkRenderPass renderPass, std::pair<uint32_t, uint32_t> size, const AllocatedImage& colorImage, bool useOld);
	void recreate(VkRenderPass renderPass, std::pair<uint32_t, uint32_t> size, const AllocatedImage& msaaImage);

	inline std::vector<AllocatedImage>& getImages() { return images; }
	inline std::vector<VkFramebuffer>& getFramebuffers() { return framebuffers; }
	inline std::vector<VkSemaphore>& getImageSemaphores() { return imageSemaphores; }

private:
	std::vector<AllocatedImage> images;
	std::vector<VkFramebuffer> framebuffers;
	std::vector<VkSemaphore> imageSemaphores;

	VulkanDevice* device;
};

#endif
