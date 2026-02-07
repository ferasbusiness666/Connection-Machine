#include "vulkanImageSwapchain.h"
#include "gpu/mainRenderer.h"
#include "gpu/renderer/frameManager.h"

ImageSwapchain::Semaphore::~Semaphore() {
	vkDestroySemaphore(MainRenderer::get().getVulkanInstance().getDevice()->getDevice(), semaphore, nullptr);
}

void ImageSwapchain::init(VulkanDevice* device) {
	this->device = device;

	// create image semaphores
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	imageSemaphores.resize(FRAMES_IN_FLIGHT);
	for (uint32_t i = 0; i < imageSemaphores.size(); ++i) {
		VkSemaphore semaphore;
		vkCreateSemaphore(device->getDevice(), &semaphoreInfo, nullptr, &semaphore);
		imageSemaphores[i] = std::make_shared<Semaphore>(semaphore);
	}
}

void ImageSwapchain::cleanup() {
	for (unsigned int i = 0; i < images.size(); i++) {
		vkDestroyFramebuffer(device->getDevice(), framebuffers[i], nullptr);
	}
	framebuffers.clear();
	images.clear();

	imageSemaphores.clear();
}

const bool vsync = true;

void ImageSwapchain::create(VkRenderPass renderPass, std::pair<uint32_t, uint32_t> size, const AllocatedImage& msaaImage, bool useOld) {
	images.resize(imageSemaphores.size());
	framebuffers.resize(images.size());
	for (unsigned int i = 0; i < images.size(); i++) {
		if (size.first == 0 || size.second == 0) return;

		VkExtent3D imageSize = {size.first, size.second, 1};
		VkSampleCountFlagBits msaaSamples = device->getMaxUsableSampleCount();

		// Create resolve image (non-MSAA, for ImGui to sample)
		images[i] = std::make_shared<AllocatedImage>(
			device,
			imageSize,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			false,
			VK_SAMPLE_COUNT_1_BIT
		);

		// Create framebuffer with both attachments
		std::array<VkImageView, 2> attachments = {
			msaaImage.imageView,    	// Color attachment (MSAA)
			images[i]->imageView	// Resolve attachment (non-MSAA)
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = size.first;
		framebufferInfo.height = size.second;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device->getDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void ImageSwapchain::recreate(VkRenderPass renderPass, std::pair<uint32_t, uint32_t> size, const AllocatedImage& msaaImage) {
	for (unsigned int i = 0; i < images.size(); i++) {
		vkDestroyFramebuffer(device->getDevice(), framebuffers[i], nullptr);
	}
	framebuffers.clear();
	images.clear();
	create(renderPass, size, msaaImage, true);
}
