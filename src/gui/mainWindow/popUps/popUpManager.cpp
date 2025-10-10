#include "popUpManager.h"

#include "gui/helper/eventPasser.h"
#include "gui/mainWindow/mainWindow.h"
#include "environment/environment.h"
#include "gui/helper/saveCallback.h"

#include <SDL3/SDL.h>

void PopUpManager::addOptionsPopUp(const std::string& message, const std::vector<std::pair<std::string, std::function<void()>>>& options, bool blocking) {
	std::pair<Rml::Element*, std::function<void()>> popUpData = createPopUp(blocking);
	Rml::Element* popUpRoot = popUpData.first;
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
			[deleteFunc = popUpData.second, func = option.second](Rml::Event& event) {
				auto funcCopy = func;
				deleteFunc();
				funcCopy();
			}
		));
		setPositionButton->SetClass("pop-up-action", true);
		actionsElement->AppendChild(std::move(setPositionButton));
	}
}

std::pair<Rml::Element*, std::function<void()>> PopUpManager::createPopUp(bool blocking) {
	if (blocking || true) {
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
		return {popUpOverlay, [popUpOverlay, allPopUps](){
			Rml::Element* allPopUpsCopy = allPopUps;
			allPopUpsCopy->RemoveChild(popUpOverlay);
			if (!allPopUpsCopy->HasChildNodes()) allPopUpsCopy->SetClass("invisible", true);
		}};
	}
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
    auto [overlay, closePopup] = createPopUp(true);

    Rml::ElementList windowList;
    overlay->GetElementsByClassName(windowList, "pop-up-window");
    if (windowList.empty()) return;
    Rml::Element* window = windowList.front();

    Rml::Element* title = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("p"));
    title->SetInnerRML("Please give us any feedback!");
    title->SetClass("popup-title", true);

    Rml::Element* textarea = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("textarea"));
    textarea->SetAttribute("rows", "5");
    textarea->SetAttribute("cols", "40");
    textarea->SetClass("popup-textarea", true);
	textarea->SetClass("surface-pop", true);

    Rml::Element* submitButton = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("button"));
    submitButton->SetInnerRML("Happy Submit");
    submitButton->SetClass("popup-button", true);
	submitButton->AddEventListener(Rml::EventId::Click, new EventPasser(
	    [deleteFunc = closePopup, textarea](Rml::Event& event) {
	        // Retrieve the value from the textarea
	        auto* textareaControl = dynamic_cast<Rml::ElementFormControlTextArea*>(textarea);
			if (textareaControl)	
				logInfo("Live text: {}","",textareaControl->GetValue());
			logInfo("Feeling: happy","");
	        // Close the popup
	        deleteFunc();
	    }
	));

	Rml::Element* submitButton2 = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("button"));
    submitButton2->SetInnerRML("Medium Submit");
    submitButton2->SetClass("popup-button", true);
	submitButton2->AddEventListener(Rml::EventId::Click, new EventPasser(
	    [deleteFunc = closePopup, textarea](Rml::Event& event) {
	        // Retrieve the value from the textarea
	        auto* textareaControl = dynamic_cast<Rml::ElementFormControlTextArea*>(textarea);
			if (textareaControl)	
				logInfo("Live text: {}","",textareaControl->GetValue());
			logInfo("Feeling: medium","");
	        // Close the popup
	        deleteFunc();
	    }
	));

	Rml::Element* submitButton3 = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("button"));
    submitButton3->SetInnerRML("Sad Submit");
    submitButton3->SetClass("popup-button", true);
	submitButton3->AddEventListener(Rml::EventId::Click, new EventPasser(
	    [deleteFunc = closePopup, textarea](Rml::Event& event) {
	        // Retrieve the value from the textarea
	        auto* textareaControl = dynamic_cast<Rml::ElementFormControlTextArea*>(textarea);
			if (textareaControl)	
				logInfo("Live text: {}","",textareaControl->GetValue());
			logInfo("Feeling: sad","");
	        // Close the popup
	        deleteFunc();
	    }
	));

}