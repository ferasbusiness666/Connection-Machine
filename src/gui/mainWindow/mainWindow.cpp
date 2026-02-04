#include "mainWindow.h"
#include <SDL3/SDL_video.h>

#include "app.h"

#include "backend/settings/settings.h"
// #include "computerAPI/directoryManager.h"
#include "environment/environment.h"
#include "gui/mainWindow/widgets/circuitViewWidget.h"
#include "gui/mainWindow/widgets/selectorWidget.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

MainWindow::MainWindow() : SdlWindow("Connection Machine"), environment(true), toolManagerManager(environment), widgetIdProvider(1)/*, popUpManager(*this)*/ {
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

	WidgetId widgetId1 = widgetIdProvider.getNewId();
	widgets.emplace(widgetId1, std::make_unique<CircuitViewWidget>(widgetId1, *this));
	WidgetId widgetId2 = widgetIdProvider.getNewId();
	widgets.emplace(widgetId2, std::make_unique<SelectorWidget>(widgetId2, *this));
}

MainWindow::~MainWindow() = default;

void MainWindow::doUpdate() {
	for (std::pair<const WidgetId, std::unique_ptr<Widget>>& widget : widgets) {
		widget.second->doUpdate();
	}
	environment.getBlockRenderDataFeeder().doBlockTextureUpdates();
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

	for (std::pair<const WidgetId, std::unique_ptr<Widget>>& widget : widgets) {
		widget.second->processEvent(event);
	}

	if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) {
		applyUiScale(uiScale);
	}
}

void MainWindow::render() {
	bool open = true;
	ImGuiIO& io = ImGui::GetIO();

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("DockSpace", &open, window_flags);
	ImGui::PopStyleVar(2);

	if (ImGui::DockBuilderGetNode(ImGui::GetID("MyDockspace")) == NULL) {
		ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id);

		ImGuiID dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
		ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, NULL, &dock_main_id);
		ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, NULL, &dock_main_id);
		ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.20f, NULL, &dock_main_id);

		ImGui::DockBuilderDockWindow("James_1", dock_id_left);
		ImGui::DockBuilderDockWindow("James_2", dock_main_id);
		ImGui::DockBuilderDockWindow("James_3", dock_id_right);
		ImGui::DockBuilderDockWindow("James_4", dock_id_bottom);
		ImGui::DockBuilderFinish(dockspace_id);
	}

	ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0);
	ImGui::End();

	for (std::pair<const WidgetId, std::unique_ptr<Widget>>& widget : widgets) {
		widget.second->doRendering();
	}
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
