#ifndef sdlWindow_h
#define sdlWindow_h

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include "gpu/mainRendererDefs.h"

class SdlWindow;

namespace App {
	void runLoop();
	void registerWindow(std::shared_ptr<SdlWindow>& window);
}

class SdlWindow {
	friend void App::runLoop();
	friend void App::registerWindow(std::shared_ptr<SdlWindow>& window);
	friend bool resizingEventWatcher(void* data, SDL_Event* event);
public:
	SdlWindow(const std::string& name, unsigned int width = 800, unsigned int height = 600);
	// SdlWindow(SDL_Window* handle);
	virtual ~SdlWindow();

	float getWindowScalingSize() const { return windowScalingSize; }
	std::string getName() const;
	std::pair<uint32_t, uint32_t> getSize() const;

	WindowId getWindowId() const { return windowId; }
	SDL_Window& getHandle() { return *handle; }
	SDL_Window* getHandlePtr() { return handle; }

	bool recieveEvent(SDL_Event& event);
	bool isThisMyEvent(const SDL_Event& event) const;
	void toggleBorderlessFullscreen();

	void sendKillEvent();
	void instantKillEvent();
	bool isKilled() const { return killed; }

	VkSurfaceKHR createVkSurface(VkInstance instance);
	void doRendering(std::function<void(std::shared_ptr<void>)> preserveForFrame); // called by vulkan

	nlohmann::json dumpWindowState() const;

protected:
	// return false when the window does not want to close.
	// (This will only work when forced is false and so windows should clean up all their resources if forced is true!)
	virtual bool killWindow(bool forced) { return true; }
	virtual void render(std::function<void(std::shared_ptr<void>)> preserveForFrame) { }
	virtual void processEvent(SDL_Event& event) { } // called by recieveEvent
	virtual nlohmann::json dumpState() const { return "dumpState not overridden"; }

private:
	void kill(bool forced); // having this does not allow the use to forget to call killWindow on the sub class!

	bool killed = false;
	SDL_Window* handle = nullptr;
	float windowScalingSize;

	std::mutex renderingMux;

	// Vulkan stuff
	VkInstance vkInstance = nullptr;
	std::optional<VkSurfaceKHR> vkSurface;
	WindowId windowId;
};

#endif
