#ifndef vulkanImageSwapchain_h
#define vulkanImageSwapchain_h

#include "gpu/vulkanDevice.h"

class ImageSwapchain {
public:
	struct Semaphore {
		Semaphore(VkSemaphore semaphore) : semaphore(semaphore) {}
		~Semaphore();
		VkSemaphore semaphore;
	};
	void init(VulkanDevice& device);
	void cleanup();

	void create(VkRenderPass renderPass, std::pair<uint32_t, uint32_t> size, const AllocatedImage& colorImage, bool useOld);
	void recreate(VkRenderPass renderPass, std::pair<uint32_t, uint32_t> size, const AllocatedImage& msaaImage);

	inline std::vector<std::shared_ptr<AllocatedImage>>& getImages() { return images; }
	inline std::vector<VkFramebuffer>& getFramebuffers() { return framebuffers; }
	inline std::vector<std::shared_ptr<Semaphore>>& getImageSemaphores() { return imageSemaphores; }

private:
	std::vector<std::shared_ptr<AllocatedImage>> images;
	std::vector<VkFramebuffer> framebuffers;
	std::vector<std::shared_ptr<Semaphore>> imageSemaphores;

	VulkanDevice* device;
};

#endif /* vulkanImageSwapchain_h */
