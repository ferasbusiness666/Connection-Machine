#ifndef app_h
#define app_h

class SdlWindow;
class SDL_Window;

namespace App {
	void kill();

	std::shared_ptr<SdlWindow> registerWindow(SDL_Window* handle);
	std::shared_ptr<SdlWindow> registerWindow(const std::string& windowName);
	std::shared_ptr<SdlWindow> registerWindow(const std::string& windowName, unsigned int width, unsigned int height);

	void runLoop();
	void startTryingToQuit();
	void stopTryingToQuit();

	nlohmann::json dumpState();
};

#endif /* app_h */
