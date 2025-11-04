#include "popUpManager.h"

#include "gui/helper/eventPasser.h"
#include "gui/mainWindow/mainWindow.h"
#include "environment/environment.h"
#include "gui/helper/saveCallback.h"
#include "computerAPI/directoryManager.h"
#include "app.h"

#include <RmlUi/Debugger.h>

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
				deleteFunc();
				func();
			}
		));
		setPositionButton->SetClass("pop-up-action", true);
		actionsElement->AppendChild(std::move(setPositionButton));
	}
}

std::pair<Rml::Element*, std::function<void()>> PopUpManager::createPopUp(bool blocking, const std::string& windowName, unsigned int width, unsigned int height) {
	if (blocking) {
		Rml::Element* allPopUps = mainWindow->getRmlDocument()->GetElementById("all-pop-ups");
		allPopUps->SetClass("invisible", false);
		assert(allPopUps);
		Rml::Element* popUpOverlay = allPopUps->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
		popUpOverlay->SetClass("pop-up-overlay", true);
		Rml::Element* popUpOverlayInternal = popUpOverlay->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
		popUpOverlayInternal->SetClass("pop-up-overlay-internal", true);
		Rml::Element* popUpWindow = popUpOverlayInternal->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
		popUpWindow->SetClass("pop-up-window", true);
		popUpWindow->SetClass("pop-up-window-blocked", true);
		popUpWindow->SetClass("bordered", true);
		popUpWindow->SetClass("bg-3", true);
		return {popUpOverlay, [popUpOverlay, allPopUps](){
			App::get().queForEndOfUpdate([popUpOverlay, allPopUps](){
				allPopUps->RemoveChild(popUpOverlay);
				if (!allPopUps->HasChildNodes()) allPopUps->SetClass("invisible", true);
			});
		}};
	} else {
		SdlWindow* sdlWindow = App::get().registerWindow(windowName, width, height).get();
		WindowId windowId = MainRenderer::get().registerWindow(sdlWindow);
		Rml::Context* rmlContext = Rml::CreateContext("popup_" + std::to_string(windowId), Rml::Vector2i(sdlWindow->getSize().first, sdlWindow->getSize().second));
		if (rmlContext) {
			Rml::ElementDocument* rmlDocument = rmlContext->LoadDocument(DirectoryManager::getResourceDirectory().generic_string() + "/gui/mainWindow/popUpWindow/popUpWindow.rml");
			sdlWindow->setRenderFunction([windowId, rmlContext](){
				RmlRenderInterface* rmlRenderInterface = dynamic_cast<RmlRenderInterface*>(Rml::GetRenderInterface());
				if (rmlRenderInterface) {
					rmlContext->Update();
					rmlRenderInterface->setWindowToRenderOn(windowId);
					MainRenderer::get().prepareForRmlRender(windowId);
					rmlContext->Render();
					MainRenderer::get().endRmlRender(windowId);
				}
			});
			sdlWindow->setRecieveEventFunction(
				[windowId, rmlContext, sdlWindow](SDL_Event& event){
					if (sdlWindow->isThisMyEvent(event)) {
						if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
							Rml::RemoveContext(rmlContext->GetName());
							MainRenderer::get().deregisterWindow(windowId);
							App::get().deregisterWindow(sdlWindow);
							return true;
						}

						RmlSDL::InputEventHandler(rmlContext, sdlWindow->getHandle(), event, sdlWindow->getWindowScalingSize());

						// let renderer know we if resized the window
						if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
							MainRenderer::get().resizeWindow(windowId, { event.window.data1, event.window.data2 });
							rmlContext->Update();
						}
						return true;
					}
					return false;
				}
			);
			rmlDocument->Show();
			return {rmlDocument, [windowId, rmlContext, sdlWindow](){
				App::get().queForEndOfUpdate([windowId, rmlContext, sdlWindow](){
					Rml::RemoveContext(rmlContext->GetName());
					MainRenderer::get().deregisterWindow(windowId);
					App::get().deregisterWindow(sdlWindow);
				});
			}};
		}
		logError("Failed to create new window for popup :(");
		return {nullptr, nullptr};
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

void set_invisible(Rml::Element* element, bool invisible){
	if (invisible){
		element->SetClass("invisible",true);
		for (int i = 0; i < element->GetNumChildren(); ++i){
			element->GetChild(i)->SetClass("invisible",true);
		}
	} else {
		element->SetClass("invisible",false);
		for (int i = 0; i < element->GetNumChildren(); ++i){
			element->GetChild(i)->SetClass("invisible",false);
		}
	}
	
}

void PopUpManager::aboutConnectionMachine() {
	auto [overlay, closePopup] = createPopUp(true);
	Rml::ElementList windowList;
    overlay->GetElementsByClassName(windowList, "pop-up-window");
    if (windowList.empty()) return;
    Rml::Element* window = windowList.front();
	window->SetAttribute("style", "display: flex; flex-direction: column; width: 650px; height: 500px; background-color:#303030;");

	Rml::Element* leftandright = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
	leftandright->SetAttribute("style", "display: flex; flex-direction: row; width: 650px; height: 500px; background-color:#303030;");

	Rml::Element* leftbuffer = leftandright->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
	leftbuffer->SetAttribute("style", "display: flex; flex-direction: column; width: 20px; height: 300px; background-color:#303030;");

	Rml::Element* leftdiv = leftandright->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
	leftdiv->SetAttribute("style", "display: flex; flex-direction: column; width: 450px; height: 300px; background-color:#303030;");
	//Rml::Element* rightdiv = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));


	Rml::Element* title = leftdiv->AppendChild(mainWindow->getRmlDocument()->CreateElement("p"));
    title->SetInnerRML("About Connection Machine");
    title->SetClass("popup-title", true);

	Rml::Element* body = leftdiv->AppendChild(mainWindow->getRmlDocument()->CreateElement("p"));
    body->SetInnerRML(
		"Connection Machine is an open source project aiming to create an application for designing and simulating logic graph systems.");
    body->SetClass("popup-title", true);


	Rml::Element* body2 = leftdiv->AppendChild(mainWindow->getRmlDocument()->CreateElement("p"));
    body2->SetInnerRML(
		"<br /> github: https://github.com/Martian-Technologies/<br />Connection-Machine <br />  \
		website: https://connection-machine.com ");
    body2->SetClass("popup-title", true);

	
	Rml::Element* rightbuffer = leftandright->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
	rightbuffer->SetAttribute("style", "display: flex; flex-direction: column; width: 30px; height: 300px; background-color:#303030;");

    Rml::Element* rightdiv = leftandright->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
    rightdiv->SetAttribute("style", "display: flex; flex-direction: column; width: 150px; height: 450px; background-color:#303030;");

    // Add a logo (adjust the src path to your actual asset)
    Rml::Element* logo = rightdiv->AppendChild(mainWindow->getRmlDocument()->CreateElement("img"));
    logo->SetAttribute("src", "../../gateIcon.png");
    logo->SetAttribute("style", "width: 100px; height: 100px; margin-bottom: 20px;");

    // Add contributor names
    Rml::Element* contributorsTitle = rightdiv->AppendChild(mainWindow->getRmlDocument()->CreateElement("p"));
    contributorsTitle->SetInnerRML("Contributors:");
    contributorsTitle->SetClass("popup-subtitle", true);

    Rml::Element* contributorsList = rightdiv->AppendChild(mainWindow->getRmlDocument()->CreateElement("ul"));

	std::vector<std::string> contributors = {
        "Ben Herman",
		"Niknal",
		"Jack J",
		"Connor K",
		"James P",
		"Gregory L",
		"Sam C",
		"August B",
		"Nick C",
		"Matthew D",
		"Dante L",
		"Tiffany C",
		"Patrick C",
		"Nathan C",
		"Alek K ",
    };

	// Rml::Element* contributorsList = rightdiv->AppendChild(mainWindow->getRmlDocument()->CreateElement("li"));
	std::string all_contributors = "";
    for (const auto& name : contributors){
		all_contributors = all_contributors + name.c_str() + "<br />";
	}
	contributorsList->SetInnerRML(all_contributors);
	contributorsList->SetAttribute("style", "margin: 4px 0;");


	Rml::Element* close = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("button"));
    close->SetInnerRML("Close");
    close->SetClass("popup-button", true);
	close->AddEventListener(Rml::EventId::Click, new EventPasser(
	    [deleteFunc = closePopup](Rml::Event& event) {
	        deleteFunc();
	    }
	));

}

void PopUpManager::addFeedbackPopup() { //feature request, bug report, feature complaint, feedback
    auto [overlay, closePopup] = createPopUp(true);

    Rml::ElementList windowList;
    overlay->GetElementsByClassName(windowList, "pop-up-window");
    if (windowList.empty()) return;
    Rml::Element* window = windowList.front();

    
	Rml::Element* title = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("p"));
    title->SetInnerRML("Please give us any feedback!");
    title->SetClass("popup-title", true);

	Rml::ElementPtr el_ptr = Rml::Factory::InstanceElement(
	    window, "select", "select", Rml::XMLAttributes());
	
	Rml::Element* el = el_ptr.get();
	
	auto* dropdown = dynamic_cast<Rml::ElementFormControlSelect*>(el);
	if (!dropdown)
	    return; 
	
	dropdown->SetClass("popup-dropdown", true);
	dropdown->SetClass("surface-pop", true);
	dropdown->SetAttribute("id", "feedback_select");
	dropdown->SetAttribute("style", "width: 160px; height: 20px; background-color:#303030;");
	
	int feedback1 = dropdown->Add("Feedback", "feedback");
	int feedback2 = dropdown->Add("Bug Report", "bug");
	int feedback3 = dropdown->Add("Feature Request", "request");
	int feedback4 = dropdown->Add("Feature Complaint", "complaint");
	dropdown->SetSelection(0);
	
	window->AppendChild(std::move(el_ptr));
	Rml::Element* BugReportContainerText = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
	Rml::Element* BugReportContainer = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));

	Rml::Element* feedback1use = dropdown->GetOption(feedback1);
	feedback1use->SetAttribute("style", "width: 160px; height: 20px; background-color:#303030;");
	Rml::Element* feedback2use = dropdown->GetOption(feedback2);
	feedback2use->SetAttribute("style", "width: 160px; height: 20px; background-color:#303030;");
	Rml::Element* feedback3use = dropdown->GetOption(feedback3);
	feedback3use->SetAttribute("style", "width: 160px; height: 20px; background-color:#303030;");
	Rml::Element* feedback4use = dropdown->GetOption(feedback4);
	feedback4use->SetAttribute("style", "width: 160px; height: 20px; background-color:#303030;");

	dropdown->AddEventListener(Rml::EventId::Change, new EventPasser(
	    [BugReportContainer,BugReportContainerText,window](Rml::Event& e) {
	        auto* select = dynamic_cast<Rml::ElementFormControlSelect*>(e.GetTargetElement());
			if (select){
				if (select->GetValue() == "Bug Report" || select->GetValue() == "bug"){
					set_invisible(BugReportContainer,false);
					set_invisible(BugReportContainerText,false);
					window->SetClass("pop-up-window-blocked", false);
					window->SetClass("pop-up-window-blocked-bug-report", true);
				} else {
					set_invisible(BugReportContainer,true);
					set_invisible(BugReportContainerText,true);
					window->SetClass("pop-up-window-blocked", true);
					window->SetClass("pop-up-window-blocked-bug-report", false);
				}
			}
	        
	    }
	));
	
	BugReportContainerText->SetClass("bug-report-container-text", true);
	BugReportContainerText->SetAttribute(
	    "style",
	    "display: flex; justify-content: space-between; align-items: center; width: 100%; margin-top: 10px;"
	);
	
	
	BugReportContainer->SetClass("bug-report-container", true);
	BugReportContainer->SetAttribute(
	    "style",
	    "display: flex; justify-content: space-between; align-items: center; width: 100%; margin-top: 10px;"
	);
	Rml::Element* checkboxtext = BugReportContainerText->AppendChild(mainWindow->getRmlDocument()->CreateElement("text"));
	Rml::Element* checkboxtext2 = BugReportContainerText->AppendChild(mainWindow->getRmlDocument()->CreateElement("text"));
	Rml::Element* checkboxtext3 = BugReportContainerText->AppendChild(mainWindow->getRmlDocument()->CreateElement("text"));
	Rml::Element* checkboxtext4 = BugReportContainerText->AppendChild(mainWindow->getRmlDocument()->CreateElement("text"));
	Rml::Element* checkboxtext5 = BugReportContainerText->AppendChild(mainWindow->getRmlDocument()->CreateElement("text"));
	Rml::Element* checkbox = BugReportContainer->AppendChild(mainWindow->getRmlDocument()->CreateElement("input"));
	Rml::Element* checkbox2 = BugReportContainer->AppendChild(mainWindow->getRmlDocument()->CreateElement("input"));
	Rml::Element* checkbox3 = BugReportContainer->AppendChild(mainWindow->getRmlDocument()->CreateElement("input"));
	Rml::Element* checkbox4 = BugReportContainer->AppendChild(mainWindow->getRmlDocument()->CreateElement("input"));
	Rml::Element* checkbox5 = BugReportContainer->AppendChild(mainWindow->getRmlDocument()->CreateElement("input"));

	checkboxtext->SetInnerRML("Crashes");
	checkboxtext2->SetInnerRML("Files");
	checkboxtext3->SetInnerRML("Settings");
	checkboxtext4->SetInnerRML("Logic");
	checkboxtext5->SetInnerRML("User Interface");
	checkbox->SetAttribute("type","checkbox");
	checkbox2->SetAttribute("type","checkbox");
	checkbox3->SetAttribute("type","checkbox");
	checkbox4->SetAttribute("type","checkbox");
	checkbox5->SetAttribute("type","checkbox");
	set_invisible(BugReportContainer,true);
	set_invisible(BugReportContainerText,true);
	
	// Rml::Element* StepsToReproduceBugtitle = BugReportContainer->AppendChild(mainWindow->getRmlDocument()->CreateElement("p"));
	// StepsToReproduceBugtitle->SetInnerRML("Steps to reproduce bug:");

	// Rml::Element* StepsToReproduceBugtextarea = BugReportContainer->AppendChild(mainWindow->getRmlDocument()->CreateElement("textarea"));
    // StepsToReproduceBugtextarea->SetAttribute("rows", "5");
    // StepsToReproduceBugtextarea->SetAttribute("cols", "40");
    // StepsToReproduceBugtextarea->SetClass("popup-textarea", true);
	// StepsToReproduceBugtextarea->SetClass("surface-pop", true);
	// StepsToReproduceBugtextarea->SetInnerRML("Enter Here.");
	// StepsToReproduceBugtextarea->AddEventListener(Rml::EventId::Keydown, new EventPasser(
	//     [ StepsToReproduceBugtextarea](Rml::Event& event) {
	//         auto* textareaControl = dynamic_cast<Rml::ElementFormControlTextArea*>(StepsToReproduceBugtextarea);
	// 		if (textareaControl->GetValue() != ""){
	// 			StepsToReproduceBugtextarea->SetInnerRML("");
	// 		} else {
	// 			StepsToReproduceBugtextarea->SetInnerRML("Enter Here.");
	// 		}
			
	//     }
	// ));
	// StepsToReproduceBugtextarea->AddEventListener(Rml::EventId::Keyup, new EventPasser(
	//     [ StepsToReproduceBugtextarea](Rml::Event& event) {
	//         auto* textareaControl = dynamic_cast<Rml::ElementFormControlTextArea*>(StepsToReproduceBugtextarea);
	// 		if (textareaControl->GetValue() != ""){
	// 			StepsToReproduceBugtextarea->SetInnerRML("");
	// 		} else {
	// 			StepsToReproduceBugtextarea->SetInnerRML("Enter Here.");
	// 		}
	//     }
	// ));

    Rml::Element* textarea = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("textarea"));
    textarea->SetAttribute("rows", "5");
    textarea->SetAttribute("cols", "40");
    textarea->SetClass("popup-textarea", true);
	textarea->SetClass("surface-pop", true);
	textarea->SetInnerRML("Enter Here.");
	textarea->AddEventListener(Rml::EventId::Keydown, new EventPasser(
	    [ textarea](Rml::Event& event) {
	        auto* textareaControl = dynamic_cast<Rml::ElementFormControlTextArea*>(textarea);
			if (textareaControl->GetValue() != ""){
				textarea->SetInnerRML("");
			} else {
				textarea->SetInnerRML("Enter Here.");
			}
			
	    }
	));
	textarea->AddEventListener(Rml::EventId::Keyup, new EventPasser(
	    [ textarea](Rml::Event& event) {
	        auto* textareaControl = dynamic_cast<Rml::ElementFormControlTextArea*>(textarea);
			if (textareaControl->GetValue() != ""){
				textarea->SetInnerRML("");
			} else {
				textarea->SetInnerRML("Enter Here.");
			}
	    }
	));

	Rml::Element* buttonContainer = window->AppendChild(mainWindow->getRmlDocument()->CreateElement("div"));
	buttonContainer->SetClass("popup-button-container", true);
	buttonContainer->SetAttribute(
	    "style",
	    "display: flex; justify-content: space-between; align-items: center; width: 100%; margin-top: 10px;"
	);
    Rml::Element* submitButton = buttonContainer->AppendChild(mainWindow->getRmlDocument()->CreateElement("button"));
    submitButton->SetInnerRML("Happy Submit");
    submitButton->SetClass("popup-button", true);
	submitButton->AddEventListener(Rml::EventId::Click, new EventPasser(
	    [deleteFunc = closePopup, textarea, dropdown,BugReportContainer,BugReportContainerText](Rml::Event& event) {
	        // Retrieve the value from the textarea
	        auto* textareaControl = dynamic_cast<Rml::ElementFormControlTextArea*>(textarea);
			auto* select = dynamic_cast<Rml::ElementFormControlSelect*>(dropdown);
			if (textareaControl && select)
				logInfo("Dropdown Select: {}","",select->GetValue());
				logInfo("Live text: {}","",textareaControl->GetValue());

			if (select->GetValue() == "bug"){
				for (int i = 0; i < BugReportContainer->GetNumChildren(); ++i){
					logInfo("Value {} is {}","",BugReportContainerText->GetChild(i)->GetInnerRML(),BugReportContainer->GetChild(i)->HasAttribute("checked"));
				}
			}
			logInfo("Feeling: happy","");
	        // Close the popup
	        deleteFunc();
	    }
	));

	Rml::Element* submitButton2 = buttonContainer->AppendChild(mainWindow->getRmlDocument()->CreateElement("button"));
    submitButton2->SetInnerRML("Medium Submit");
    submitButton2->SetClass("popup-button", true);
	submitButton2->AddEventListener(Rml::EventId::Click, new EventPasser(
	    [deleteFunc = closePopup, textarea, dropdown,BugReportContainer,BugReportContainerText](Rml::Event& event) {
	        // Retrieve the value from the textarea
	        auto* textareaControl = dynamic_cast<Rml::ElementFormControlTextArea*>(textarea);
			auto* select = dynamic_cast<Rml::ElementFormControlSelect*>(dropdown);
			if (textareaControl && select)	
				logInfo("Dropdown Select: {}","",select->GetValue());
				logInfo("Live text: {}","",textareaControl->GetValue());
			
				if (select->GetValue() == "bug"){
				for (int i = 0; i < BugReportContainer->GetNumChildren(); ++i){
					logInfo("Value {} is {}","",BugReportContainerText->GetChild(i)->GetInnerRML(),BugReportContainer->GetChild(i)->HasAttribute("checked"));
				}
			}
			logInfo("Feeling: medium","");
	        // Close the popup
	        deleteFunc();
	    }
	));

	Rml::Element* submitButton3 = buttonContainer->AppendChild(mainWindow->getRmlDocument()->CreateElement("button"));
    submitButton3->SetInnerRML("Sad Submit");
    submitButton3->SetClass("popup-button", true);
	submitButton3->AddEventListener(Rml::EventId::Click, new EventPasser(
	    [deleteFunc = closePopup, textarea, dropdown,BugReportContainer,BugReportContainerText](Rml::Event& event) {
	        // Retrieve the value from the textarea
	        auto* textareaControl = dynamic_cast<Rml::ElementFormControlTextArea*>(textarea);
			auto* select = dynamic_cast<Rml::ElementFormControlSelect*>(dropdown);
			if (textareaControl && select)	
				logInfo("Dropdown Select: {}","",select->GetValue());
				logInfo("Live text: {}","",textareaControl->GetValue());

			if (select->GetValue() == "bug"){
				for (int i = 0; i < BugReportContainer->GetNumChildren(); ++i){
					logInfo("Value {} is {}","",BugReportContainerText->GetChild(i)->GetInnerRML(),BugReportContainer->GetChild(i)->HasAttribute("checked"));
				}
			}
			
			logInfo("Feeling: sad","");
	        // Close the popup
	        deleteFunc();
	    }
	));

}