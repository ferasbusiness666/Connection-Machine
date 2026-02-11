#include "mainWindow.h"

#include <SDL3/SDL_video.h>

#include "imgui/imgui_internal.h"
#include "imgui/imgui_notify.h"

#include "app.h"
#include "backend/settings/settings.h"
#include "environment/environment.h"
#include "gui/helper/keybindHelpers.h"

// widgets
#include "widgets/blockSelectorWidget.h"
#include "widgets/circuitViewWidget.h"
#include "widgets/toolSelectorWidget.h"

MainWindow::MainWindow() : SdlWindow("Connection Machine"), environment(true), toolManagerManager(environment), widgetIdProvider(1) {
	const double* initialUiScale = Settings::get<SettingType::DECIMAL>("Appearance/UI Scale");
	applyUiScale(initialUiScale ? static_cast<float>(*initialUiScale) : 1.0f);
	Settings::registerListener<SettingType::DECIMAL>("Appearance/UI Scale", [this](const double& value) { applyUiScale(static_cast<float>(value)); });

	Settings::registerListener<SettingType::FILE_PATH>("Appearance/Font", [this](const std::string& fontFilePath) {
		// Rml::LoadFontFace(fontFilePath);
		logInfo("loaded, {}", "", fontFilePath);
	});

	createWidget<CircuitViewWidget>().newCircuit();
	createWidget<ToolSelectorWidget>();
	createWidget<BlockSelectorWidget>();
}

MainWindow::~MainWindow() = default;

void MainWindow::log(const std::string& message) {
	ImGui::InsertNotification({ ImGuiToastType_Info, 3000, "%s", message.c_str() });
}

void MainWindow::logError(const std::string& message) {
	ImGui::InsertNotification({ ImGuiToastType_Error, 3000, "%s", message.c_str() });
}

bool MainWindow::isPressingKeybind(const Keybind& keybind, bool repeat) const {
	if (lastUpdatedFrame >= frameIndex.load() && (keybind.getKeybind() & (~ImGuiKey::ImGuiMod_Mask_)) != ImGuiKey::ImGuiKey_None) return false;
	return ::isPressingKeybind(keybind, pressedKeys);
}

bool MainWindow::isPressingKeybind(const std::string& settingKey, bool repeat) const {
	const Keybind* keybind = Settings::get<SettingType::KEYBIND>(settingKey);
	if (!keybind) return false;
	return isPressingKeybind(*keybind, repeat);
}

void MainWindow::doUpdate() {
	pressedKeys = ::getPressedKeys(getHandle());
	if (isPressingKeybind("Keybinds/Editing/Paste")) {
		toolManagerManager.setTool("paste tool");
	}
	if (isPressingKeybind("Keybinds/Editing/Tools/State Changer")) {
		toolManagerManager.setTool("state changer");
	}
	if (isPressingKeybind("Keybinds/Editing/Tools/Connection")) {
		toolManagerManager.setTool("connection");
		toolManagerManager.setMode("Single");
	}
	if (isPressingKeybind("Keybinds/Editing/Tools/Tensor Connect")) {
		toolManagerManager.setTool("connection");
		toolManagerManager.setMode("Tensor");
	}
	if (isPressingKeybind("Keybinds/Editing/Tools/Move")) {
		toolManagerManager.setTool("move");
	}
	if (isPressingKeybind("Keybinds/Editing/Tools/Mode Changer")) {
		toolManagerManager.setTool("mode changer");
	}
	if (isPressingKeybind("Keybinds/Editing/Tools/Placement")) {
		toolManagerManager.setTool("placement");
		toolManagerManager.setMode("Single");
	}
	if (isPressingKeybind("Keybinds/Editing/Tools/Area Placement")) {
		toolManagerManager.setTool("placement");
		toolManagerManager.setMode("Area");
	}
	if (isPressingKeybind("Keybinds/Editing/Tools/Selection Maker")) {
		toolManagerManager.setTool("selection maker");
	}
	if (isPressingKeybind("Keybinds/Window/Toggle Fullscreen")) {
		toggleBorderlessFullscreen();
	}
	if (isPressingKeybind("Keybinds/Window/Increase UI Scale")) {
		offsetUiScale(kUiScaleStep);
	}
	if (isPressingKeybind("Keybinds/Window/Decrease UI Scale")) {
		offsetUiScale(-kUiScaleStep);
	}
	if (isPressingKeybind("Keybinds/Window/Reset UI Scale")) {
		applyUiScale(1.0f);
	}

	for (std::pair<const WidgetId, std::unique_ptr<Widget>>& widget : widgets) {
		widget.second->doUpdate();
	}
	environment.getBlockRenderDataFeeder().doBlockTextureUpdates();
	lastUpdatedFrame = frameIndex.load();
}

void MainWindow::processEvent(SDL_Event& event) {
	// if (event.type == SDL_EVENT_KEYMAP_CHANGED) {
	// 	if (settingsWindow) {
	// 		settingsWindow->reloadContent();
	// 	}
	// }

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
	// global styling
	ImGuiStyle& style = ImGui::GetStyle();
	{std::lock_guard lock(uiScaleMux);
	style.FontScaleMain = uiScale;}
	{
		frameIndex.fetch_add(1);
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

			dockMainId = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
			dockLeftId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Left, 0.25f, NULL, &dockMainId);
			dockRightId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.25f, NULL, &dockMainId);
			dockBottomId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Down, 0.25f, NULL, &dockMainId);
			ImGui::DockBuilderFinish(dockspace_id);
		}

		ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0);
		ImGui::End();

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Connnection Machine")) {
				if (ImGui::MenuItem("About")) {
					logInfo("ImGui branch!");
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Quit Connnection Machine")) {
					App::runOnMain([this]() { App::startTryingToQuit(); });
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New Circuit", "Ctrl+N")) {
					App::runOnMain([this]() { environment.getBackend().createCircuit(); });
				}
				if (ImGui::MenuItem("New Viewport")) {
					App::runOnMain([this]() {
						WidgetId widgetId = widgetIdProvider.getNewId();
						widgets.emplace(widgetId, std::make_unique<CircuitViewWidget>(widgetId, *this));
					});
				}
				if (ImGui::MenuItem("New Window")) {
					App::runOnMain([this]() { App::makeWindow<MainWindow>(); });
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Open...", "Ctrl+O")) { }
				if (ImGui::MenuItem("Save", "Ctrl+S")) { }
				if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) { }
				if (ImGui::MenuItem("Exit")) { }
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit")) {
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View")) {
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help")) {
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		for (std::pair<const WidgetId, std::unique_ptr<Widget>>& widget : widgets) {
			widget.second->doRendering();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f));
		ImGui::RenderNotifications();
		ImGui::PopStyleColor(1);
		ImGui::PopStyleVar(1);
	}
}

void MainWindow::offsetUiScale(double delta) {
	const double* storedScale = Settings::get<SettingType::DECIMAL>("Appearance/UI Scale");
	double currentScale;
	{
		std::lock_guard lock(uiScaleMux);
		currentScale = storedScale ? *storedScale : static_cast<double>(uiScale);
	}
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
	if (std::abs(static_cast<double>(clamped) - static_cast<double>(scale)) > 1e-6) {
		const double* storedScale = Settings::get<SettingType::DECIMAL>("Appearance/UI Scale");
		if (!storedScale || std::abs(*storedScale - clamped) > 1e-6) {
			Settings::set<SettingType::DECIMAL>("Appearance/UI Scale", clamped);
		}
	}
	{
		std::lock_guard lock(uiScaleMux);
		uiScale = clamped;
	}
}
