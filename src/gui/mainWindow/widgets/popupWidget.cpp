#include "popupWidget.h"

#include "app.h"
#include "util/preprocessors.h"
#include "../mainWindow.h"

PopupWidget::PopupWidget(WidgetId widgetId, MainWindow& mainWindow, std::string message, const std::vector<std::pair<std::string, std::function<void()>>>& buttons) :
	Widget(widgetId, mainWindow), message(message), buttons(buttons) {
	ImGui::OpenPopup(("###" + getWidgetIdStr()).c_str());
}

void PopupWidget::render() {
	getMainWindow().pushWindowStyling();
	ifGui (ImGui::BeginPopupModal(("###" + getWidgetIdStr()).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize),
		getMainWindow().popWindowStyling();
	) {
		ImGui::Text("%s", message.c_str());
		for (const auto& button : buttons) {
			if (ImGui::Button(button.first.c_str())){
				ImGui::CloseCurrentPopup();
				App::runOnMain([fn = button.second, this](){
					getMainWindow().destroyWidget(getWidgetId());
					if (fn) fn();
				});
			}
		}
		ImGui::EndPopup();
	}
}
