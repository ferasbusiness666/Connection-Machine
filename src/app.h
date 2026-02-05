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

	float getDetlaTime();
	void runLoop();
	void startTryingToQuit();
	void stopTryingToQuit();

	void runOnMain_blocking(std::function<void()> func);
	void runOnMain(std::function<void()> func);

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
