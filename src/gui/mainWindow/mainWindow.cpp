#include "mainWindow.h"
#include <SDL3/SDL_video.h>

#include "app.h"

#include "backend/settings/settings.h"
// #include "computerAPI/directoryManager.h"
#include "environment/environment.h"
#include "imgui/imgui.h"

MainWindow::MainWindow() : SdlWindow("Connection Machine"), environment(true), toolManagerManager(environment)/*, popUpManager(*this)*/ {
	// keybind handling
	// rmlDocument->AddEventListener(Rml::EventId::Keydown, &keybindHandler);
	// keybindHandler.addListener("Keybinds/Editing/Paste", [this]() { toolManagerManager.setTool("paste tool"); });
	// keybindHandler.addListener("Keybinds/Editing/Tools/State Changer", [this]() { toolManagerManager.setTool("state changer"); });
	// keybindHandler.addListener("Keybinds/Editing/Tools/Connection", [this]() {
	// 	toolManagerManager.setTool("connection");
	// 	toolManagerManager.setMode("Single");
	// });
	// keybindHandler.addListener("Keybinds/Editing/Tools/Tensor Connect", [this]() {
	// 	toolManagerManager.setTool("connection");
	// 	toolManagerManager.setMode("Tensor");
	// });
	// keybindHandler.addListener("Keybinds/Editing/Tools/Move", [this]() { toolManagerManager.setTool("move"); });
	// keybindHandler.addListener("Keybinds/Editing/Tools/Mode Changer", [this]() { toolManagerManager.setTool("mode changer"); });
	// keybindHandler.addListener("Keybinds/Editing/Tools/Placement", [this]() {
	// 	toolManagerManager.setTool("placement");
	// 	toolManagerManager.setMode("Single");
	// });
	// keybindHandler.addListener("Keybinds/Editing/Tools/Area Placement", [this]() {
	// 	toolManagerManager.setTool("placement");
	// 	toolManagerManager.setMode("Area");
	// });
	// keybindHandler.addListener("Keybinds/Editing/Tools/Selection Maker", [this]() { toolManagerManager.setTool("selection maker"); });
	// keybindHandler.addListener("Keybinds/Window/Toggle Fullscreen", [this]() { sdlWindow->toggleBorderlessFullscreen(); });
	// keybindHandler.addListener("Keybinds/Window/Increase UI Scale", [this]() { offsetUiScale(kUiScaleStep); });
	// keybindHandler.addListener("Keybinds/Window/Decrease UI Scale", [this]() { offsetUiScale(-kUiScaleStep); });
	// keybindHandler.addListener("Keybinds/Window/Reset UI Scale", [this]() { applyUiScale(1.0f); });

	const double* initialUiScale = Settings::get<SettingType::DECIMAL>("Appearance/UI Scale");
	applyUiScale(initialUiScale ? static_cast<float>(*initialUiScale) : 1.0f);
	Settings::registerListener<SettingType::DECIMAL>("Appearance/UI Scale", [this](const double& value) { applyUiScale(static_cast<float>(value)); });

	Settings::registerListener<SettingType::FILE_PATH>("Appearance/Font", [this](const std::string& fontFilePath) {
		// Rml::LoadFontFace(fontFilePath);
		logInfo("loaded, {}", "", fontFilePath);
	});

}

void MainWindow::processEvent(SDL_Event& event) {
	// if (event.type == SDL_EVENT_KEYMAP_CHANGED) {
	// 	if (settingsWindow) {
	// 		settingsWindow->reloadContent();
	// 	}
	// }

	// check if we want this event // done before the event is sent
	// if (!isThisMyEvent(event)) return;// event.type == SDL_EVENT_KEYMAP_CHANGED;

	// send event to RML
	// RmlSDL::InputEventHandler(rmlContext, sdlWindow->getHandle(), event, getSdlWindowScalingSize());

	if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) {
		applyUiScale(uiScale);
	}
}

void MainWindow::render() {
	ImGui::DockSpaceOverViewport();

	bool my_tool_active = true;
	ImGui::Begin("My First Tool", &my_tool_active, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
			if (ImGui::MenuItem("Save", "Ctrl+S"))   { /* Do stuff */ }
			if (ImGui::MenuItem("Close", "Ctrl+W"))  { my_tool_active = false; }
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	// Generate samples and plot them
	float samples[100];
	for (int n = 0; n < 100; n++)
		samples[n] = sinf(n * 0.2f + ImGui::GetTime() * 1.5f);
	ImGui::PlotLines("Samples", samples, 100);

	// Display contents in a scrolling region
	ImGui::TextColored(ImVec4(1,1,0,1), "Important Stuff");
	ImGui::BeginChild("Scrolling");
	for (int n = 0; n < 50; n++)
		ImGui::Text("%04d: Some text", n);
	ImGui::EndChild();
	ImGui::End();
}

void MainWindow::offsetUiScale(double delta) {
	const double* storedScale = Settings::get<SettingType::DECIMAL>("Appearance/UI Scale");
	const double currentScale = storedScale ? *storedScale : static_cast<double>(uiScale);
	const double targetScale = std::clamp(currentScale + delta, kUiScaleMin, kUiScaleMax);
	if (std::abs(targetScale - currentScale) < 1e-6) {
		applyUiScale(static_cast<float>(targetScale));
		return;
	}
	if (!Settings::set<SettingType::DECIMAL>("Appearance/UI Scale", targetScale)) {
		applyUiScale(static_cast<float>(targetScale));
	}
}

void MainWindow::applyUiScale(float scale) {
	const float clamped = std::clamp(scale, static_cast<float>(kUiScaleMin), static_cast<float>(kUiScaleMax));
	if (std::abs(static_cast<double>(clamped) - static_cast<double>(scale)) > 1e-6 && !uiScaleSettingUpdateInProgress) {
		const double* storedScale = Settings::get<SettingType::DECIMAL>("Appearance/UI Scale");
		if (!storedScale || std::abs(*storedScale - clamped) > 1e-6) {
			uiScaleSettingUpdateInProgress = true;
			Settings::set<SettingType::DECIMAL>("Appearance/UI Scale", clamped);
			uiScaleSettingUpdateInProgress = false;
		}
	}
	uiScale = clamped;
	float displayScale = 1.0f;
	displayScale = SDL_GetWindowDisplayScale(getHandlePtr());
	if (!(displayScale > 0.0f)) {
		displayScale = 1.0f;
	}
	// rmlContext->SetDensityIndependentPixelRatio(displayScale * uiScale);

	// if (settingsWindow) {
	// 	settingsWindow->reloadContent();
	// }

	// for (auto circuitViewWidget : circuitViewWidgets) {
	// 	circuitViewWidget->handleResize();
	// }
}
