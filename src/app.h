#ifndef app_h
#define app_h

#include "environment/environment.h"
#include "gui/mainWindow/mainWindow.h"
#include "gui/rml/rmlInstance.h"
#include "gui/rml/rmlRenderInterface.h"
#include "gui/rml/rmlSystemInterface.h"
#include "gui/sdl/sdlInstance.h"

class App {
public:
	static App& get();
	static void kill();

	// do not call
	App();
	~App();

	std::shared_ptr<SdlWindow> registerWindow(const std::string& windowName);
	void deregisterWindow(std::shared_ptr<SdlWindow>& sdlWindow);
	void deregisterWindow(const SdlWindow* sdlWindow);

	void newMainWindow();
	bool closeMainWindow(const MainWindow* mainWindow);

	void runLoop();
	void startTryingToQuit();
	void stopTryingToQuit();

private:
	Environment environment;

	RmlRenderInterface rmlRenderInterface;
	RmlSystemInterface rmlSystemInterface;

	SdlInstance sdl;
	std::optional<RmlInstance> rml;

	std::vector<std::shared_ptr<SdlWindow>> sdlWindows;
	std::vector<std::unique_ptr<MainWindow>> windows; // we could make this just a vector later, I don't want to deal with moving + threads
	std::vector<const MainWindow*> windowsToDestroy;
	std::vector<MainWindow*> newlyCreatedWindowsNext;
	std::vector<MainWindow*> newlyCreatedWindows;
	bool running = false;
	bool tryingToQuit = false;
	unsigned int tasksToFinishToQuit = 0;
};

#endif /* app_h */
