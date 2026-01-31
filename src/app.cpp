#include "app.h"

#ifdef TRACY_PROFILER
#include <tracy/Tracy.hpp>
#endif


#include "gui/sdl/sdlWindow.h"
#include "network/network.h"
#include "util/version.h"
#include "gpu/renderer/imgui/imGuiRenderer.h"
#include "gui/sdl/sdlInstance.h"
#include "gpu/mainRenderer.h"

#include <SDL3/SDL.h>

// gloable app values
std::optional<SdlInstance> sdl;
std::set<std::shared_ptr<SdlWindow>> windows;
bool running = false;
bool tryingToQuit = false;
unsigned int tasksToFinishToQuit = 0;

void App::kill() {
	logInfo("Killing App", "App");
	logInfo("Shutting down App", "App");
	for (std::shared_ptr<SdlWindow> window : windows) {
		if (!window->isKilled()) {
			logError("SDL window \"{}\" was not killed.", "App", window->getName());
		}
	}
	windows.clear();
	MainRenderer::kill();
	Network::kill();
}


void App::registerWindow(std::shared_ptr<SdlWindow>& window) {
	windows.emplace(window);
}

#ifdef TRACY_PROFILER
const char* const addLoopTracyName = "appLoop";
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
	while (running) {
		if (windows.empty()) App::kill(); // Ff there are not more windows kill the app!

		// Wait for the next event (so we don't broork the cpu)
		bool gotEvent = SDL_WaitEventTimeout(nullptr, 50);
#ifdef TRACY_PROFILER
		FrameMarkStart(addLoopTracyName);
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
		// { // do we need to do this???
		// 	std::vector<std::weak_ptr<SdlWindow>> windowsToRender;
		// 	for (const std::shared_ptr<SdlWindow>& window : windows) {
		// 		windowsToRender.push_back(window);
		// 	}
		// 	for (unsigned int i = 0; i < windowsToRender.size(); ++i) {
		// 		std::shared_ptr<SdlWindow> thisWindow = windowsToRender[i].lock();
		// 		if (!thisWindow) continue;
		// 		thisWindow->render();
		// 	}
		// }
#ifdef TRACY_PROFILER
		FrameMarkEnd(addLoopTracyName);
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
	if (tasksToFinishToQuit == 0) running = false;
}

void App::stopTryingToQuit() { tryingToQuit = false; }

nlohmann::json App::dumpState() /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json stateJson;
	stateJson["windows"] = nlohmann::json::array();
	for (const std::shared_ptr<SdlWindow>& window : windows) {
		stateJson["windows"] = window->dumpWindowState();
	}
	stateJson["version"] = getCurrentVersion().toString();
	return stateJson;
}
