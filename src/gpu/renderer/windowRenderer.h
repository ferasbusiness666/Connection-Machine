#ifndef windowRenderer_h
#define windowRenderer_h

#include <RmlUi/Core/RenderInterface.h>

#include "gpu/renderer/imgui/imGuiRenderer.h"
#include "gui/sdl/sdlWindow.h"

#include "gpu/abstractions/vulkanSwapchain.h"
#include "gpu/renderer/frameManager.h"
#include "gpu/renderer/viewport/viewportRenderData.h"
#include "gpu/renderer/viewport/viewportRenderer.h"
#include "gpu/renderer/rml/rmlRenderer.h"

class WindowRenderer {
public:
	WindowRenderer(SdlWindow* sdlWindow);
	~WindowRenderer();

	// no copy
	WindowRenderer(const WindowRenderer&) = delete;
	WindowRenderer& operator=(const WindowRenderer&) = delete;

public:
	void resize(std::pair<uint32_t, uint32_t> windowSize);

	RmlRenderer& getRmlRenderer() { return rmlRenderer; }
	const RmlRenderer& getRmlRenderer() const { return rmlRenderer; }

	ImGuiRenderer& getImGuiRenderer() { return *imGuiRenderer; }
	const ImGuiRenderer& getImGuiRenderer() const { return *imGuiRenderer; }

	void registerViewportRenderData(ViewportRenderData* viewportRenderData);
	void deregisterViewportRenderData(ViewportRenderData* viewportRenderData);
	bool hasViewportRenderData(ViewportRenderData* viewportRenderData);

	inline VulkanDevice* getDevice() { return device; }

private:
	void createColorResources();
	void cleanupColorResources();
	void renderToCommandBuffer(Frame& frame, uint32_t imageIndex);
	void createRenderPass();
	void recreateSwapchain();

private:
	// screen
	Swapchain swapchain;
	std::atomic<bool> swapchainRecreationNeeded = false;
	std::pair<uint32_t, uint32_t> windowSize;
	std::mutex windowSizeMux;

	// main vulkan
	VkSurfaceKHR surface;
	VkRenderPass renderPass;

	// subrenderers
	RmlRenderer rmlRenderer;
	std::optional<ImGuiRenderer> imGuiRenderer;
	ViewportRenderer viewportRenderer;

	// render loop
	FrameManager frames;
	std::thread renderThread;
	std::atomic<bool> running = false;
	void renderLoop();

	// connected viewport render interfaces
	std::set<ViewportRenderData*> viewportRenderDatas;
	std::mutex viewportRenderersMux;

	// handles
	SdlWindow* sdlWindow;
	VulkanDevice* device;


	AllocatedImage colorImage;
};

#endif /* windowRenderer_h */
