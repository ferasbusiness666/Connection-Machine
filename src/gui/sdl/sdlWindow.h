#ifndef sdlWindow_h
#define sdlWindow_h

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

namespace App { void runLoop(); }

class SdlWindow {
	friend void App::runLoop();
	friend bool resizingEventWatcher(void* data, SDL_Event* event);
public:
	SdlWindow(const std::string& name, unsigned int width = 800, unsigned int height = 600);
	SdlWindow(SDL_Window* handle);
	virtual ~SdlWindow();

	bool recieveEvent(SDL_Event& event);
	bool isThisMyEvent(const SDL_Event& event) const;

	void sendKillEvent();
	void instantKillEvent();
	bool isKilled() const { return killed; }

	std::string getName() const;

	VkSurfaceKHR createVkSurface(VkInstance instance);
	std::pair<uint32_t, uint32_t> getSize() const;

	float getWindowScalingSize() const { return windowScalingSize; }

	SDL_Window& getHandle() { return *handle; }
	SDL_Window* getHandlePtr() { return handle; }

	void toggleBorderlessFullscreen();

	nlohmann::json dumpWindowState() const;

protected:
	// return false when the window does not want to close.
	// (This will only work when forced is false and so windows should clean up all their resources if forced is true!)
	virtual bool killWindow(bool forced) { return true; }
	virtual void render() { } // called by vulkan
	virtual void processEvent(SDL_Event& event) { } // called by recieveEvent
	virtual nlohmann::json dumpState() const { return "dumpState not overridden"; }

private:
	void kill(bool forced); // having this does not allow the use to forget to call killWindow on the sub class!

	bool killed = false;
	SDL_Window* handle = nullptr;
	float windowScalingSize;

	// Vulkan stuff
	VkInstance vkInstance = nullptr;
	std::optional<VkSurfaceKHR> vkSurface;
};

#endif
