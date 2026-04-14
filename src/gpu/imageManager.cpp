#include "imageManager.h"
#include "imgui/imgui_impl_vulkan.h"
#include "mainRenderer.h"

std::pair<VkDescriptorSet, std::vector<std::shared_ptr<void>>> ImageManager::getImage(const std::string& path, WindowId windowId) {
	{
		auto iter = loadedImages.find(path);
		if (iter != loadedImages.end()) {
			auto iter2 = iter->second.find(windowId);
			if (iter2 == iter->second.end()) {
				auto lifetimeObjects = iter2->second->lifetimeObjects;
				lifetimeObjects.push_back(iter2->second);
				return {iter2->second->descriptorSet, lifetimeObjects};
			}
		}
	}

	auto windowIter = MainRenderer::get().windowRenderers.find(windowId);
	if (windowIter == MainRenderer::get().windowRenderers.end()) return { VK_NULL_HANDLE, {} };

	int width, height, channels;
	stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (!pixels) return { VK_NULL_HANDLE, {} };
	VkExtent3D size{};
	size.width = width;
	size.height = height;
	size.depth = 1;

	std::shared_ptr<AllocatedImage> image = std::make_shared<AllocatedImage>(
		MainRenderer::get().getVulkanInstance().getDevice(), pixels, size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT
	);

	if (sampler == nullptr) {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		VkSampler vkSampler;
		vkCreateSampler(MainRenderer::get().getVulkanInstance().getDevice().getDevice(), &samplerInfo, nullptr, &vkSampler);
		sampler = std::make_shared<ViewportRenderer::Sampler>(vkSampler);
	}

	std::shared_ptr<ImGuiRenderer::ImGuiDescriptorSet> ds = loadedImages[path].emplace(windowId,
		std::make_shared<ImGuiRenderer::ImGuiDescriptorSet>(
			ImGui_ImplVulkan_AddTexture(sampler->sampler, image->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
			windowIter->second.getImGuiRenderer(),
			std::vector<std::shared_ptr<void>>{ image, sampler }
		)
	).first->second;
	auto lifetimeObjects = ds->lifetimeObjects;
	lifetimeObjects.push_back(ds);
	return {ds->descriptorSet, lifetimeObjects};
}

void ImageManager::clearWindow(WindowId windowId) {
	for (auto& loadedImage : loadedImages) {
		auto iter = loadedImage.second.find(windowId);
		if (iter != loadedImage.second.end()) {
			loadedImage.second.erase(iter);
		}
	}
}
