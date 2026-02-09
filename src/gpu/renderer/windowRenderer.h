#ifndef windowRenderer_h
#define windowRenderer_h

#include "gpu/renderer/imgui/imGuiRenderer.h"
#include "gui/sdl/sdlWindow.h"

#include "gpu/abstractions/vulkanSwapchain.h"
#include "gpu/renderer/frameManager.h"

class WindowRenderer {
public:
	WindowRenderer(WindowId windowId, SdlWindow* sdlWindow);
	~WindowRenderer();

	// no copy
	WindowRenderer(const WindowRenderer&) = delete;
	WindowRenderer& operator=(const WindowRenderer&) = delete;

	void resize(std::pair<uint32_t, uint32_t> windowSize);

	ImGuiRenderer& getImGuiRenderer() { return *imGuiRenderer; }
	const ImGuiRenderer& getImGuiRenderer() const { return *imGuiRenderer; }
	void setImGuiRenderFunc(std::function<void()> imGuiRenderFunc);

	void addSemaphore(VkSemaphore semaphore, const std::vector<std::shared_ptr<void>>& lifetimeObjects) {
		semaphoreForThisFrame.push_back(semaphore);
		addLifetimeObjects(lifetimeObjects);
	}
	void addLifetimeObjects(const std::vector<std::shared_ptr<void>>& lifetimeObjects) {
		for (const std::shared_ptr<void>& obj : lifetimeObjects) frames.getCurrentFrame()->lifetime.push(obj);
	}
	VulkanDevice* getDevice() { return device; }

	std::pair<VkDescriptorSet, std::vector<std::shared_ptr<void>>> getBlockTextureArrayLayer_ImGui(unsigned int layer) {
		updateImGuiBlockTextureArrayLayers(); // just make sure its up to date
		std::lock_guard lock(imGuiBlockTextureArrayLayersMux);
		if (imGuiBlockTextureArrayLayers.size() <= layer) {
			logError("layer index of {} out of range for imGuiBlockTextureArrayLayers with {} layers", "WindowRenderer::getBlockTextureArrayLayer_ImGui", layer, imGuiBlockTextureArrayLayers.size());
			return { VK_NULL_HANDLE, {} };
		}
		return std::make_pair(imGuiBlockTextureArrayLayers[layer]->descriptorSet, std::vector<std::shared_ptr<void>>{ imGuiBlockTextureArrayLayers[layer] } );
	}

private:
	void createColorResources();
	void renderToCommandBuffer(Frame& frame, uint32_t imageIndex);
	void createRenderPass();
	void recreateSwapchain();

private:
	// screen
	WindowId windowId;
	Swapchain swapchain;
	std::atomic<bool> swapchainRecreationNeeded = false;
	std::pair<uint32_t, uint32_t> windowSize;
	std::mutex windowSizeMux;

	// main vulkan
	VkSurfaceKHR surface;
	VkRenderPass renderPass;
	std::vector<VkSemaphore> semaphoreForThisFrame;

	// subrenderers
	std::optional<ImGuiRenderer> imGuiRenderer;
	std::mutex imGuiRenderFuncMux;
	std::function<void()> imGuiRenderFunc = nullptr;

	// render loop
	FrameManager frames;
	std::thread renderThread;
	std::atomic<bool> running = false;
	std::atomic<bool> renderLoopStopped = true;
	void renderLoop();

	void updateImGuiBlockTextureArrayLayers();
	std::mutex imGuiBlockTextureArrayLayersMux;
	std::vector<std::shared_ptr<ImGuiRenderer::ImGuiDescriptorSet>> imGuiBlockTextureArrayLayers;
	std::shared_ptr<BlockTextureArray> blockTextureArrayImage;

	// handles
	SdlWindow* sdlWindow;
	VulkanDevice* device;

	std::optional<AllocatedImage> colorImage;
};

#endif /* windowRenderer_h */
