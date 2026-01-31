#ifndef imGuiRenderer_h
#define imGuiRenderer_h

#include <SDL3/SDL_events.h>
#include <vulkan/vulkan_core.h>

class SDL_Window;
class ImGuiContext;
class VulkanInstance;
class VulkanDevice;

class ImGuiRenderer {
public:
	ImGuiRenderer(SDL_Window* window, VkRenderPass renderPass, uint32_t framesInFlight);
	~ImGuiRenderer();

	ImGuiRenderer(const ImGuiRenderer&) = delete;
	ImGuiRenderer& operator=(const ImGuiRenderer&) = delete;

	void beginFrame();
	void endFrame(VkCommandBuffer cmd);

	ImGuiContext* getContext() const { return m_context; }

	static ImGuiRenderer* getImGuiRenderer(SDL_Window* sdlWindow);
	static void allProcessEvent(const SDL_Event& e);

private:
	void createDescriptorPool();
	void initImGui();

	SDL_Window* m_mainWindow;
	VkRenderPass m_renderPass;
	uint32_t m_framesInFlight;

	ImGuiContext* m_context;
	VkDescriptorPool m_imguiDescriptorPool;
};

#endif /* imGuiRenderer_h */
