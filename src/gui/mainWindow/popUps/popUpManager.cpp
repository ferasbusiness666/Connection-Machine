#include "popUpManager.h"

#include "gui/helper/eventPasser.h"
#include "gui/mainWindow/mainWindow.h"
#include "environment/environment.h"

#include <SDL3/SDL.h>

void PopUpManager::addOptionsPopUp(const std::string& message, const std::vector<std::pair<std::string, std::function<void()>>>& options, bool blocking) {
	createPopUp(message, blocking);
	mainWindow->getRmlDocument()->GetElementById("pop-up-text")->SetInnerRML(message);
	Rml::Element* actionsElement = mainWindow->getRmlDocument()->GetElementById("pop-up-actions");
	while (actionsElement->HasChildNodes()) { actionsElement->RemoveChild(actionsElement->GetChild(0)); }
	for (const auto& option : options) {
		Rml::ElementPtr setPositionButton = mainWindow->getRmlDocument()->CreateElement("button");
		setPositionButton->AppendChild(std::move(mainWindow->getRmlDocument()->CreateTextNode(option.first)));
		setPositionButton->AddEventListener(Rml::EventId::Click, new EventPasser(
			[this, func = option.second](Rml::Event& event) {
				mainWindow->getRmlDocument()->GetElementById("pop-up-overlay")->SetClass("invisible", true);
				func();
			}
		));
		setPositionButton->SetClass("pop-up-action", true);
		actionsElement->AppendChild(std::move(setPositionButton));
	}
}

Rml::Element* PopUpManager::createPopUp(const std::string& message, bool blocking) {
	return mainWindow->getRmlDocument()->GetElementById("pop-up-overlay");
}

void SaveCallback(void* userData, const char* const* filePaths, int filter) {
	std::pair<CircuitFileManager*, std::string>* data = (std::pair<CircuitFileManager*, std::string>*)userData;
	if (filePaths && filePaths[0]) {
		std::string filePath = filePaths[0];
		if (data->first->getSavePath(data->second) != nullptr)
			logWarning("This circuit " + data->second + " will be saved with a new UUID");
		data->first->saveToFile(filePath, data->second);
	} else {
		std::cout << "File dialog canceled." << std::endl;
	}
	delete data;
}

void PopUpManager::savePopUp(const std::string& circuitUUID) {
	if (!mainWindow->getEnvironment()->getCircuitFileManager().save(circuitUUID)) {
		// if failed to save the circuit with out a path
		static const SDL_DialogFileFilter filters[] = {
			{ "Circuit Files",  ".cir" }
		};
		std::pair<CircuitFileManager*, std::string>* data = new std::pair<CircuitFileManager*, std::string>(&mainWindow->getEnvironment()->getCircuitFileManager(), circuitUUID);
		SDL_ShowSaveFileDialog(SaveCallback, data, mainWindow->getSdlWindow(), filters, 1, nullptr);
	}
}

void PopUpManager::saveAsPopUp(const std::string& circuitUUID) {
	static const SDL_DialogFileFilter filters[] = {
		{ "Circuit Files",  ".cir" }
	};
	std::pair<CircuitFileManager*, std::string>* data = new std::pair<CircuitFileManager*, std::string>(&mainWindow->getEnvironment()->getCircuitFileManager(), circuitUUID);
	SDL_ShowSaveFileDialog(SaveCallback, data, mainWindow->getSdlWindow(), filters, 1, nullptr);
}
