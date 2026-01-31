#ifndef app_h
#define app_h

class SdlWindow;
class SDL_Window;

namespace App {
	void kill();
	template<class WindowType, class... Args>
	requires std::derived_from<WindowType, SdlWindow>
	std::shared_ptr<SdlWindow> makeWindow(Args&&... args);
	void registerWindow(std::shared_ptr<SdlWindow>& window);

	void runLoop();
	void startTryingToQuit();
	void stopTryingToQuit();

	nlohmann::json dumpState();
};

template<class WindowType, class... Args>
requires std::derived_from<WindowType, SdlWindow>
std::shared_ptr<SdlWindow> App::makeWindow(Args&&... args) {
	std::shared_ptr<SdlWindow> window = std::make_shared<WindowType>(std::forward<Args>(args)...);
	registerWindow(window);
	return window;
}


#endif /* app_h */
