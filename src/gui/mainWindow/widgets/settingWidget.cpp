#include "settingWidget.h"

#include "app.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "util/preprocessors.h"
#include "../mainWindow.h"
#include "util/version.h"
#include "util/algorithm.h"

SettingWidget::SettingWidget(WidgetId widgetId, MainWindow& mainWindow) : Widget(widgetId, mainWindow) { }

struct SettingTreeNode {
	std::map<std::string, SettingTreeNode> children;
	bool leaf;
	void addPath(std::span<std::string> path) {
		if (path.size() == 0) {
			leaf = true;
			return;
		}
		children[path.front()].addPath(std::span(path.begin() + 1, path.end()));
	}
	void render(const std::string& path, const std::string& name) const {
		if (leaf) {
			ImGui::Indent(10);
			ImGui::SameLine();
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, -2.5));
			switch (Settings::getType(path)) {
				case SettingType::VOID: {
					ImGui::Text("VOID SETTING TYPE");
				} break;
				case SettingType::STRING: {
					std::string str = valueOr(Settings::get<SettingType::STRING>(path), std::string(""));
					if (ImGui::InputText("##ID", &str)) {
						App::runOnMain([this, str, path]() {
							Settings::set<SettingType::STRING>(path, str);
						});
					}
				} break;
				case SettingType::INT: {
					int num = valueOr(Settings::get<SettingType::INT>(path), 0);
					if (ImGui::InputScalar("##ID", ImGuiDataType_S32, &num)) {
						App::runOnMain([this, num, path]() {
							Settings::set<SettingType::INT>(path, num);
						});
					}
				} break;
				case SettingType::UINT: {
					unsigned int num = valueOr(Settings::get<SettingType::UINT>(path), 0u);
					if (ImGui::InputScalar("##ID", ImGuiDataType_U32, &num)) {
						App::runOnMain([this, num, path]() {
							Settings::set<SettingType::UINT>(path, num);
						});
					}
				} break;
				case SettingType::DECIMAL: {
					double num = valueOr(Settings::get<SettingType::DECIMAL>(path), 0.0);
					if (ImGui::InputScalar("##ID", ImGuiDataType_Double, &num)) {
						App::runOnMain([this, num, path]() {
							Settings::set<SettingType::DECIMAL>(path, num);
						});
					}
				} break;
				case SettingType::BOOL: {
					bool boolean = valueOr(Settings::get<SettingType::BOOL>(path), false);
					if (ImGui::Checkbox("##ID", &boolean)) {
						App::runOnMain([this, boolean, path]() {
							Settings::set<SettingType::BOOL>(path, boolean);
						});
					}
				} break;
				case SettingType::KEYBIND: {
					Keybind keybind = valueOr(Settings::get<SettingType::KEYBIND>(path), Keybind());
					if (ImGui::Button(keybind.toString().c_str())) {

					}
					// if (ImGui::InputText("##ID", k.c_str())) {
					// 	App::runOnMain([this, keybind, path]() {
					// 		Settings::set<SettingType::KEYBIND>(path, keybind);
					// 	});
					// }
				} break;
				case SettingType::FILE_PATH: {
					std::string filePath = valueOr(Settings::get<SettingType::FILE_PATH>(path), std::string(""));
					if (ImGui::InputText("##ID", &filePath)) {
						App::runOnMain([this, filePath, path]() {
							Settings::set<SettingType::FILE_PATH>(path, filePath);
						});
					}
				}break;
			}
			ImGui::Unindent(10);
		} else {
			for (const auto& child : children) {
				if (child.second.leaf) {
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImU32(0));
					ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImU32(0));
					ifGui (ImGui::TreeNodeEx((child.first + "##node").c_str(), ImGuiTreeNodeFlags_Leaf),
						ImGui::PopStyleColor(2);
					) {
						child.second.render(path.empty() ? child.first : (path + "/" + child.first), child.first);
						ImGui::TreePop();
					}
				} else {
					if (ImGui::TreeNodeEx((child.first + "##node").c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
						child.second.render(path.empty() ? child.first : (path + "/" + child.first), child.first);
						ImGui::TreePop();
					}
				}
			}
		}
	}
};

void SettingWidget::render() {
	ImGui::SetNextWindowDockID(getMainWindow().getDockMainId(), ImGuiCond_FirstUseEver);
	getMainWindow().setNextWindowMainDockable();
	getMainWindow().pushWindowStyling();
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ifGui (ImGui::Begin(("Setting###" + getWidgetIdStr()).c_str(), NULL),
		getMainWindow().popWindowStyling();
	) {
		static ImGuiTextFilter filter;
		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
			filter.Clear();
		}
		ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
		filter.Draw("##Filter", -FLT_MIN);
		std::lock_guard mux(Settings::getSettingsMapReadLock());
		std::vector<std::string> paths;
		for (const auto& pair : Settings::getSettingsMap().getAllKeys())  {
			if (filter.PassFilter(pair.first.c_str())) {
				paths.push_back(pair.first);
			}
		}
		SettingTreeNode root;
		for (const std::string& path : paths) {
			std::vector<std::string> pathVec = stringSplit(path, '/');
			root.addPath(std::span(pathVec.begin(), pathVec.end()));
		}
		if (ImGui::BeginChild("tree")) {
			root.render("", "");
		}
		ImGui::EndChild();
		ImGui::End();
	}
}
