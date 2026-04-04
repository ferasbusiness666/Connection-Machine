#include "aboutWidget.h"

#include "imgui/imgui_internal.h"
#include "util/preprocessors.h"
#include "../mainWindow.h"
#include "util/version.h"

AboutWidget::AboutWidget(WidgetId widgetId, MainWindow& mainWindow) : Widget(widgetId, mainWindow) { }

void AboutWidget::render() {
	getMainWindow().pushWindowStyling();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos((viewport->Size - ImVec2(400, 350)) / 2);
	ImGui::SetNextWindowSize(ImVec2(400, 350));
	ifGui (ImGui::Begin(("About###" + getWidgetIdStr()).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar),
		getMainWindow().popWindowStyling();
		ImGui::PopStyleVar();
	) {
		if (ImGui::BeginChild("Title", ImVec2(0, 45))) {
			float curScale = ImGui::GetCurrentWindow()->FontWindowScale;
			ImGui::SetWindowFontScale(curScale * 2);
			ImGui::Text("Connection Machine");
			ImGui::SetWindowFontScale(curScale);
			ImGui::SameLine();
			// ImGui::Image()
			ImGui::Text("%s", PROJECT_VERSION);
		}
		ImGui::EndChild();
		if (ImGui::BeginChild("Description", ImVec2(250, 0))) {
			ImGui::TextWrapped("Connection Machine is an open source project aiming to create an application for designing and simulating logic graph systems.");
		}
		ImGui::EndChild();
		ImGui::SameLine();
		if (ImGui::BeginChild("Contributors", ImVec2(0, 0)), ImGuiChildFlags_Borders) {
			std::vector<std::string> contributors = {
				"Ben Herman",
				"Nikita Lurye",
				"Jack Jamison",
				"Connor Kostiew",
				"Gregory Lemonnier",
				"James P",
				"Sam C",
				"Alek Krupka",
				"August Bernberg",
				"Nicholas Ciuica",
				"Matthew Durcan",
				"Tiffany Cheng",
				"Patrick Chen",
				"Nathan Chen",
				"Dante Luis"
			};
			for (const auto& name : contributors) {
				ImGui::Text("%s", name.c_str());
			}
		}
		ImGui::EndChild();
		ImGui::End();
	}
}
