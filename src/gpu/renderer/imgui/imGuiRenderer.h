#ifndef imGuiRenderer_h
#define imGuiRenderer_h

#include <vulkan/vulkan_core.h>

class SDL_Window;
union SDL_Event;
class ImGuiContext;
class VulkanInstance;
class VulkanDevice;

class ImGuiRenderer {
public:
	ImGuiRenderer(SDL_Window& window, VkRenderPass renderPass, uint32_t framesInFlight);
	~ImGuiRenderer();

	ImGuiRenderer(const ImGuiRenderer&) = delete;
	ImGuiRenderer& operator=(const ImGuiRenderer&) = delete;

	void beginFrame();
	void endFrame(VkCommandBuffer cmd);

	ImGuiContext* getContext() const { return context; }
	std::unique_lock<std::mutex> setActiveContext() const;
	void addPostFrameWork(std::function<void()>);

	static ImGuiRenderer* getImGuiRenderer(SDL_Window& sdlWindow);
	static void allProcessEvent(const SDL_Event& e);

private:
	void createDescriptorPool();
	void initImGui();

	SDL_Window& mainWindow;
	VkRenderPass renderPass;
	uint32_t framesInFlight;

	std::mutex postFrameWorkLock;
	std::vector<std::function<void()>> postFrameWork;

	ImGuiContext* context = nullptr;
	VkDescriptorPool imguiDescriptorPool;
};

#endif /* imGuiRenderer_h */
