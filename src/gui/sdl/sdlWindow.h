#ifndef sdlWindow_h
#define sdlWindow_h

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

class SdlWindow {
	friend bool resizingEventWatcher(void* data, SDL_Event* event);
public:
	SdlWindow(const std::string& name, unsigned int width = 800, unsigned int height = 600);
	SdlWindow(SDL_Window* handle);
	~SdlWindow();
	void clear();

	void setRenderFunction(std::function<void()> func) { doRender = func; }
	void setRecieveEventFunction(std::function<bool(SDL_Event&)> func) { doRecieveEvent = func; }

	void render() { if (doRender) doRender(); }
	bool recieveEvent(SDL_Event& event);
	void sendKillEvent();
	void instantKillEvent();

	bool isThisMyEvent(const SDL_Event& event);

	VkSurfaceKHR createVkSurface(VkInstance instance);
	std::pair<uint32_t, uint32_t> getSize();

	float getWindowScalingSize() const { return windowScalingSize; }

	SDL_Window* getHandle() { return handle; }

	void toggleBorderlessFullscreen();

private:
	// TODO - smart pointer with custom deleter?
	SDL_Window* handle = nullptr;

	float windowScalingSize;

	// Vulkan stuff
	VkInstance vkInstance = nullptr;
	std::optional<VkSurfaceKHR> vkSurface;
	std::function<void()> doRender;
	std::function<bool(SDL_Event&)> doRecieveEvent;
};

#endif
