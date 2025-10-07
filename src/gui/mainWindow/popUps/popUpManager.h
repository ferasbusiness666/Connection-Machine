#ifndef popUpManager_h
#define popUpManager_h

#include "RmlUi/Core/Element.h"

class MainWindow;

class PopUpManager {
public:
	PopUpManager(MainWindow* mainWindow) : mainWindow(mainWindow) {}

	void addOptionsPopUp(const std::string& message, const std::vector<std::pair<std::string, std::function<void()>>>& options, bool blocking = true);
	std::pair<Rml::Element*, std::function<void()>> createPopUp(bool blocking = true);
	void addFeedbackPopup();

	void savePopUp(const std::string& circuitUUID);
	void saveAsPopUp(const std::string& circuitUUID);

private:
	MainWindow* mainWindow;
};

#endif /* popUpManager_h */