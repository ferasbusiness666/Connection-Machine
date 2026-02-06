#include "app.h"

#ifdef TRACY_PROFILER
#include <tracy/Tracy.hpp>
#endif

#include <SDL3/SDL.h>

#include "gui/sdl/sdlWindow.h"
#include "network/network.h"
#include "util/version.h"
#include "gpu/renderer/imgui/imGuiRenderer.h"
#include "gui/sdl/sdlInstance.h"
#include "gpu/mainRenderer.h"

std::thread::id mainThreadId = std::this_thread::get_id();

// gloable app values
std::optional<SdlInstance> sdl;
std::set<std::shared_ptr<SdlWindow>> windows;
bool running = false;
bool tryingToQuit = false;
std::mutex runOnMainFunctionsMux;
std::vector<std::pair<std::thread::id, std::function<void()>>> runOnMainFunctions;
std::chrono::time_point<std::chrono::high_resolution_clock> lastUpdateTime;
std::chrono::nanoseconds detlaTime;
std::atomic<bool> killingApp = false;

void App::kill() {
	logInfo("Killing App", "App");
	for (std::shared_ptr<SdlWindow> window : windows) {
		if (!window->isKilled()) {
			logError("SDL window \"{}\" was not killed.", "App", window->getName());
		}
	}
	killingApp.store(true);
	windows.clear();
	MainRenderer::kill();
	Network::kill();
	sdl.reset();
	std::lock_guard lock(runOnMainFunctionsMux);
	assert(runOnMainFunctions.empty());
}

void App::registerWindow(std::shared_ptr<SdlWindow>& window) {
	windows.emplace(window);
	window->renderingMux.unlock();
}

float App::getDetlaTime() {
	return ((double)detlaTime.count()) / 1000000000.;
}

#ifdef TRACY_PROFILER
const char* const mainAppLoop_Tracy = "AppLoop";
#endif

void App::runLoop() {
	logInfo("Starting App loop", "App");
	if (windows.empty()) {
		logError("Killing App loop. No windows Found!", "App");
		App::kill();
		return;
	}
	// Network::get().checkForUpdates(get().windows[0]->getPopUpManager());
	running = true;
	lastUpdateTime = std::chrono::high_resolution_clock::now();
	while (running) {
		if (windows.empty()) App::kill(); // Ff there are not more windows kill the app!

		// Wait for the next event (so we don't broork the cpu)
		bool gotEvent = SDL_WaitEventTimeout(nullptr, 8);

		// first thing we need to do is run functions from other threads (mainly imgui rending)
		{
			std::lock_guard lock(runOnMainFunctionsMux);
			for (auto func : runOnMainFunctions) func.second();
			runOnMainFunctions.clear();
		}

		std::chrono::time_point<std::chrono::high_resolution_clock> updateTime = std::chrono::high_resolution_clock::now();
		detlaTime = updateTime - lastUpdateTime;
		lastUpdateTime = updateTime;
#ifdef TRACY_PROFILER
		FrameMarkStart(mainAppLoop_Tracy);
#endif
		// clean up killed windows
		std::erase_if(windows, [](const std::shared_ptr<SdlWindow>& window){ return window->isKilled(); });

		if (gotEvent) {
			// process events (TODO - should probably just have a map of window ids to windows)
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				ImGuiRenderer::allProcessEvent(event);
				switch (event.type) {
				case SDL_EVENT_QUIT: {
					// Main application quit (eg. ctrl-c in terminal)
					startTryingToQuit();
					break;
				}
				default: {
					// Send event to all windows
					std::vector<std::weak_ptr<SdlWindow>> windowsToSendEvents;
					for (const std::shared_ptr<SdlWindow>& window : windows) {
						windowsToSendEvents.push_back(window);
					}
					for (unsigned int i = 0; i < windowsToSendEvents.size(); ++i) {
						std::shared_ptr<SdlWindow> thisWindow = windowsToSendEvents[i].lock();
						if (!thisWindow) continue;
						if (std::any_of(windows.begin(), windows.end(), [windowPtr = thisWindow.get()](const std::shared_ptr<SdlWindow>& a) {
							return a.get() == windowPtr;
						})) {
							thisWindow->recieveEvent(event);
						}
					}
				}
				}
			}
		}
		for (const std::shared_ptr<SdlWindow>& window : windows) {
			window->doUpdate();
		}
#ifdef TRACY_PROFILER
		FrameMarkEnd(mainAppLoop_Tracy);
#endif
	}
}

void App::startTryingToQuit() {
	// if (tryingToQuit) return;
	// { // TEMPORARY
	// 	// do dumpstate and into file
	// 	std::string dumpStateStr = dumpState().dump(1, '\t');
	// 	std::string path = (DirectoryManager::getConfigDirectory() / ("dumpstate.json")).string();
	// 	std::ofstream file(path);
	// 	if (file.is_open()) {
	// 		file << dumpStateStr;
	// 		file.close();
	// 		logInfo("Dumped state to {}", "App", path);
	// 	} else {
	// 		logError("Could not open {} to dump state!", "App", path);
	// 	}
	// }
	// tasksToFinishToQuit = 0;
	// tryingToQuit = true;
	// auto windowIter = windows.begin();
	// while (&windowIter->get()->getEnvironment() != &environment) {
	// 	++windowIter;
	// 	if (windowIter == windows.end()) {
	// 		logError("Could not find window to save with! TODO: create new window that saves can happen in!");
	// 		return;
	// 	}
	// }
	// for (auto& fileData : environment.getCircuitFileManager().getAllFiles()) {
	// 	if (fileData.second.lastSavedEdit.empty()) continue;
	// 	std::string message = "Do you want to save:\n";
	// 	std::vector<const std::string*> toSave;
	// 	for (auto lastSavedEdit : fileData.second.lastSavedEdit) {
	// 		const SharedCircuit& circuit = environment.getBackend().getCircuitManager().getCircuit(lastSavedEdit.first);
	// 		if (!circuit->isEditable()) continue;
	// 		if (lastSavedEdit.second == circuit->getEditCount()) continue;
	// 		message += circuit->getCircuitName() + "\n";
	// 		toSave.emplace_back(&circuit->getUUID());
	// 	}
	// 	if (toSave.size() == 0) continue;
	// 	++tasksToFinishToQuit;
	// 	windowIter->get()->getPopUpManager().addOptionsPopUp(
	// 		message,
	// 		{ std::make_pair(
	// 			  "Save",
	// 			  [filePath = fileData.second.fileLocation, this]() {
	// 		logInfo("Saving {}", "", filePath);
	// 		environment.getCircuitFileManager().saveFile(filePath);
	// 		--tasksToFinishToQuit;
	// 		if (tasksToFinishToQuit == 0) running = false;
	// 	}
	// 		  ),
	// 		  std::make_pair(
	// 			  "Dont Save",
	// 			  [this]() {
	// 		logInfo("\"Dont Save\" option picked.");
	// 		--tasksToFinishToQuit;
	// 		if (tasksToFinishToQuit == 0) running = false;
	// 	}
	// 		  ),
	// 		  std::make_pair("Cancel", [this]() {
	// 		logInfo("Canceling close.");
	// 		stopTryingToQuit();
	// 	}) }
	// 	);
	// }
	// for (auto& circuit : environment.getBackend().getCircuitManager().getCircuits()) {
	// 	if (environment.getCircuitFileManager().getSavePath(circuit.second->getUUID())) continue;
	// 	if (circuit.second->isEmpty() || circuit.second->getEditCount() == 0 || !circuit.second->isEditable()) continue;
	// 	++tasksToFinishToQuit;
	// 	windowIter->get()->getPopUpManager().addOptionsPopUp(
	// 		"Do you want to save: " + circuit.second->getCircuitName(),
	// 		{ std::make_pair(
	// 			  "Save",
	// 			  [window = windowIter->get(), uuid = circuit.second->getUUID(), this]() {
	// 		logInfo("Saving circuit {}", "", uuid);
	// 		static const SDL_DialogFileFilter filters[] = { { "Circuit Files", "cir" } };
	// 		std::pair<CircuitFileManager*, std::string>* data = new std::pair<CircuitFileManager*, std::string>(&environment.getCircuitFileManager(), uuid);
	// 		SDL_ShowSaveFileDialog([](void* userData, const char* const* filePaths, int filter) {
	// 			SaveCallback(userData, filePaths, filter);
	// 			--(App::get().tasksToFinishToQuit);
	// 			if (App::get().tasksToFinishToQuit == 0) App::get().running = false;
	// 		}, data, nullptr, filters, 1, nullptr);
	// 	}
	// 		  ),
	// 		  std::make_pair(
	// 			  "Dont Save",
	// 			  [this]() {
	// 		logInfo("\"Dont Save\" option picked.");
	// 		--tasksToFinishToQuit;
	// 		if (tasksToFinishToQuit == 0) running = false;
	// 	}
	// 		  ),
	// 		  std::make_pair("Cancel", [this]() {
	// 		logInfo("Canceling close.");
	// 		stopTryingToQuit();
	// 	}) }
	// 	);
	// }
	// if (tasksToFinishToQuit == 0) running = false;
}

void App::stopTryingToQuit() { tryingToQuit = false; }

void App::doRunOnMainForThread(std::thread::id threadId) {
	std::lock_guard lock(runOnMainFunctionsMux);
	for (int i = 0; i < runOnMainFunctions.size(); i++) {
		if (runOnMainFunctions[i].first == threadId) {
			if (i + 1 == runOnMainFunctions.size()) {
				runOnMainFunctions.pop_back();
				return;
			} else {
				runOnMainFunctions[i] = runOnMainFunctions.back();
				runOnMainFunctions.pop_back();
				i--;
			}
		}
	}
}

bool App::runOnMain_blocking(std::function<void()> func) {
#ifdef TRACY_PROFILER
	ZoneScoped; // allow us to see it stuff is slow because of this.
#endif
	// if this happens to be the main thread then we can just run the function
	if (mainThreadId == std::this_thread::get_id()) {
		func();
		return true;
	}
	if (killingApp.load()) {
		std::stringstream ss;
		ss << std::this_thread::get_id();
		logWarning("runOnMain_blocking called from thread {} while app was closing! This may be a error.", "App::runOnMain_blocking", ss.str());
		return false;
	}
	std::atomic<bool> isDone{false};
	{
		std::lock_guard lock(runOnMainFunctionsMux);
		runOnMainFunctions.emplace_back(std::this_thread::get_id(), [isDone = &isDone, func](){
			func();
			isDone->store(true);
			isDone->notify_all();
		});
	}
	isDone.wait(false);
	return false;
}

void App::runOnMain(std::function<void()> func) {
	// if this happens to be the main thread then we can just run the function
	if (mainThreadId == std::this_thread::get_id()) {
		func();
		return;
	}
	runOnMainFunctions.emplace_back(std::this_thread::get_id(), func);
}

nlohmann::json App::dumpState() /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json stateJson;
	stateJson["windows"] = nlohmann::json::array();
	for (const std::shared_ptr<SdlWindow>& window : windows) {
		stateJson["windows"] = window->dumpWindowState();
	}
	stateJson["version"] = getCurrentVersion().toString();
	return stateJson;
}
