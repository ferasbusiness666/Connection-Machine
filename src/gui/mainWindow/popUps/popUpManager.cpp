#include "popUpManager.h"

#include "gui/helper/eventPasser.h"
#include "gui/mainWindow/mainWindow.h"
#include "environment/environment.h"
#include "gui/helper/saveCallback.h"

#include <SDL3/SDL.h>

void PopUpManager::addOptionsPopUp(const std::string& message, const std::vector<std::pair<std::string, std::function<void()>>>& options, bool blocking) {
	Rml::Element* popUpRoot = createPopUp(blocking);
	Rml::ElementList windowList;
	popUpRoot->GetElementsByClassName(windowList, "pop-up-window");
	Rml::Element* window = windowList.front();
	Rml::Element* text = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("span"));
	text->SetInnerRML(message);
	text->SetClass("pop-up-text", true);
	Rml::Element* actionsElement = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("span"));
	actionsElement->SetClass("pop-up-actions", true);
	for (const auto& option : options) {
		Rml::ElementPtr setPositionButton = mainWindow->getRmlDocument()->CreateElement("button");
		setPositionButton->AppendChild(std::move(mainWindow->getRmlDocument()->CreateTextNode(option.first)));
		setPositionButton->AddEventListener(Rml::EventId::Click, new EventPasser(
			[popUpRoot, func = option.second](Rml::Event& event) {
				auto funcCopy = func;
				Rml::Element* root = popUpRoot->GetParentNode();
				root->RemoveChild(popUpRoot);
				funcCopy();
				if (!root->HasChildNodes()) root->SetClass("invisible", true);
			}
		));
		setPositionButton->SetClass("pop-up-action", true);
		actionsElement->AppendChild(std::move(setPositionButton));
	}
}

Rml::Element* PopUpManager::createPopUp(bool blocking) {
	Rml::Element* allPopUps = mainWindow->getRmlDocument()->GetElementById("all-pop-ups");
	allPopUps->SetClass("invisible", false);
	assert(allPopUps);
	Rml::Element* popUpOverlay = allPopUps->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
	popUpOverlay->SetClass("pop-up-overlay", true);
	Rml::Element* popUpOverlayInternal = popUpOverlay->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
	popUpOverlayInternal->SetClass("pop-up-overlay-internal", true);
	Rml::Element* popUpWindow = popUpOverlayInternal->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
	popUpWindow->SetClass("pop-up-window", true);
	popUpWindow->SetClass("bordered", true);
	popUpWindow->SetClass("bg-3", true);
	return popUpOverlay;
}

void PopUpManager::savePopUp(const std::string& circuitUUID) {
	if (!mainWindow->getEnvironment()->getCircuitFileManager().save(circuitUUID)) {
		// if failed to save the circuit with out a path
		saveAsPopUp(circuitUUID);
	}
}

void PopUpManager::saveAsPopUp(const std::string& circuitUUID) {
	static const SDL_DialogFileFilter filters[] = {
		{ "Circuit Files",  ".cir" }
	};
	std::pair<CircuitFileManager*, std::string>* data = new std::pair<CircuitFileManager*, std::string>(&mainWindow->getEnvironment()->getCircuitFileManager(), circuitUUID);
	SDL_ShowSaveFileDialog(SaveCallback, data, nullptr, filters, 1, nullptr);
}

void PopUpManager::addFeedbackPopup() {
	Rml::Element* popUpRoot = createPopUp(true);
	Rml::ElementList windowList;
	popUpRoot->GetElementsByClassName(windowList, "pop-up-window");
	Rml::Element* window = windowList.front();

	Rml::Element* text = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("span"));
	text->SetInnerRML("Testing");
	text->SetClass("pop-up-text", true);
}