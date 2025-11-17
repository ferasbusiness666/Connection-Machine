#include "menuBar.h"

#include "gui/mainWindow/settingsWindow/settingsWindow.h"
#include "gui/mainWindow/circuitView/circuitViewWidget.h"
#include "gui/mainWindow/mainWindow.h"
#include "gui/helper/eventPasser.h"

#include "app.h"

MenuBar::MenuBar(Rml::ElementDocument* context, SettingsWindow* settingsWindow, MainWindow* window) : context(context), element(context->GetElementById("menu-bar")), settingsWindow(settingsWindow), window(window) {
	Rml::Element* element = context->GetElementById("menu-bar");
	if (element) initialize(element);
}

void MenuBar::initialize(Rml::Element* element) {
	Rml::ElementList items;
	element->GetElementsByClassName(items, "menu-clickable");

	for (auto item : items) {
		item->AddEventListener("click", new EventPasser(
			[this](Rml::Event& event) {
				triggerEvent(event.GetCurrentElement()->GetId());
			}
		));
	}
}

void MenuBar::triggerEvent(const std::string& name) {
	auto logMenuAction = [name]() { logInfo("Menu action triggered: {}", "MenuBar", name); };

	if (name == "setting") {
		logMenuAction();
		settingsWindow->toggleVisibility();
	} else if (name == "menu-feedback") {
		logMenuAction();
		window->getPopUpManager().addFeedbackPopup();
	} else if (name == "file-new") {
		logMenuAction();
		window->getActiveCircuitViewWidget()->newCircuit();
	} else if (name == "file-open") {
		logMenuAction();
		window->getActiveCircuitViewWidget()->load();
	} else if (name == "file-save") {
		logMenuAction();
		window->getActiveCircuitViewWidget()->save();
	} else if (name == "new-window") {
		logMenuAction();
		App::get().newMainWindow();
	} else if (name == "close-window") {
		logMenuAction();
		App::get().closeMainWindow(window);
	} else if (name == "about") {
		logMenuAction();
		this->window->getPopUpManager().aboutConnectionMachine();
	} else if (name == "controls") {
		logMenuAction();
		this->window->getPopUpManager().controlsConnectionMachine();
	} else {
		logWarning("Event \"{}\" not reconized", "MenuBar", name);
	}
}
