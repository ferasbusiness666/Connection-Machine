#include "popUpManager.h"

#include "gui/helper/eventPasser.h"
#include "gui/mainWindow/mainWindow.h"
#include "environment/environment.h"

#include <SDL3/SDL.h>

static const char* popUpWindowRml = R"RML(
	<div class="pop-up-overlay">\
		<div class="pop-up-overlay-internal">\
			<div class="pop-up-window bordered bg-3">\
			</div>
		</div>
	</div>
)RML";
/*
				<span id="pop-up-text"></span>\
				<div id="pop-up-actions">\
					<button class="pop-up-action" id="23">Close2</button>\
					<button class="pop-up-action" id="33">Close3</button>\
					<button class="pop-up-action" id="13">Close1</button>\
				</div>\
*/

void PopUpManager::addOptionsPopUp(const std::string& message, const std::vector<std::pair<std::string, std::function<void()>>>& options, bool blocking) {
	Rml::Element* popUpRoot = createPopUp(message, blocking);
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
				popUpRoot->GetParentNode()->RemoveChild(popUpRoot);
				funcCopy();
			}
		));
		setPositionButton->SetClass("pop-up-action", true);
		actionsElement->AppendChild(std::move(setPositionButton));
	}
}

Rml::Element* PopUpManager::createPopUp(const std::string& message, bool blocking) {
	Rml::Element* allPopUps = mainWindow->getRmlDocument()->GetElementById("all-pop-ups");
	assert(allPopUps);
	// Rml::Element* div = allPopUps->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
	// div->SetInnerRML();
	allPopUps->SetInnerRML((allPopUps->GetInnerRML()) + std::string(popUpWindowRml));
	return allPopUps->GetLastChild();
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
