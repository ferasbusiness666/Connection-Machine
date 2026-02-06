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

public:
	void resize(std::pair<uint32_t, uint32_t> windowSize);

	ImGuiRenderer& getImGuiRenderer() { return *imGuiRenderer; }
	const ImGuiRenderer& getImGuiRenderer() const { return *imGuiRenderer; }
	void setImGuiRenderFunc(std::function<void()> imGuiRenderFunc);

	void addSemaphore(VkSemaphore semaphore) { semaphoreForThisFrame.push_back(semaphore); }
	VulkanDevice* getDevice() { return device; }

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

	// handles
	SdlWindow* sdlWindow;
	VulkanDevice* device;

	AllocatedImage colorImage;
};

#endif /* windowRenderer_h */
