#include "mainWindow.h"

#include <SDL3/SDL_video.h>

#include "SDL3/SDL_dialog.h"
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

void MainWindow::destroyWidget(WidgetId widgetId) {
	std::lock_guard lock(widgetsToDestroyMux);
	widgetsToDestroy.insert(widgetId);
}

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

void MainWindow::setNextWindowMainDockable() const {
	ImGuiWindowClass wc;
	wc.ClassId = 1;
	ImGui::SetNextWindowClass(&wc);
}

void MainWindow::setNextWindowSideBarDockable() const {
	ImGuiWindowClass wc;
	wc.ClassId = 2;
	ImGui::SetNextWindowClass(&wc);
}

std::chrono::time_point<std::chrono::high_resolution_clock> lastOpenCommand = std::chrono::high_resolution_clock::now();

void LoadCallback(void* userData, const char* const* filePaths, int filter) {
	MainWindow* mainWindow = (MainWindow*)userData;
	if (filePaths && filePaths[0]) {
		std::string filePath = filePaths[0];
		std::vector<circuit_id_t> ids = mainWindow->getEnvironment().getCircuitFileManager().loadFromFile(filePath);
		logInfo("Requested load from '{}' produced {} circuit(s)", "MainWindow::LoadCallback", filePath, ids.size());
		if (ids.empty()) {
			logInfo("No circuits found in '{}'", "MainWindow::LoadCallback", filePath);
			return;
		}
		circuit_id_t id = ids.back();
		bool doSetCir = true;
		CircuitViewWidget& circuitViewWidget = mainWindow->createWidget<CircuitViewWidget>();
		for (auto& iter : mainWindow->getEnvironment().getBackend().getSimulatorManager().getSimulators()) {
			if (iter.second->getCircuitId() == id) {
				doSetCir = false;
				logInfo("Loaded circuit {}", "MainWindow::LoadCallback", id);
				circuitViewWidget.getCircuitView().setSimulator(iter.first);
				return;
			}
		}
		logInfo("Loaded circuit {}", "MainWindow::LoadCallback", id);
		circuitViewWidget.getCircuitView().setCircuit(id);
	} else {
		logInfo("File dialog canceled.", "MainWindow::LoadCallback");
	}
}

void MainWindow::loadDialog() {
	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastOpenCommand).count() < 100) return;
	static const SDL_DialogFileFilter filters[] = {
		{ "Circuit Files", "cir" },
		{ "BLIF Files", "blif" },
		{ "WASM Files", "wasm" },
	};

	logInfo("Opening load dialog for circuit import", "CircuitViewWidget::load");
	SDL_ShowOpenFileDialog(LoadCallback, this, nullptr, filters, 3, nullptr, true);
	lastOpenCommand = std::chrono::high_resolution_clock::now();
	focus();
}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
}

void MainWindow::popWindowStyling() const {
	ImGui::PopStyleVar(1);
}

void MainWindow::doUpdate() {
	std::unordered_set<WidgetId> widgetsToDestroyMoved;
	{
		std::lock_guard lock(widgetsToDestroyMux);
		widgetsToDestroyMoved = std::move(widgetsToDestroy);
		widgetsToDestroy.clear();
	}
	for (WidgetId widgetId : widgetsToDestroyMoved) {
		std::lock_guard lock(widgetsMux);
		widgets.erase(widgetId);
	}
	pressedKeys = ::getPressedKeys(getHandle());
	if (isPressingKeybind("Keybinds/File/Open")) {
		loadDialog();
	}
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

	if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != nullptr) {
		std::string filePath(event.drop.data);
		if (filePath.empty()) return;
		std::vector<circuit_id_t> ids = getEnvironment().getCircuitFileManager().loadFromFile(filePath);
		if (ids.empty()) {
			// logError("Error", "Failed to load circuit file."); // Not a error! It is valid to load 0 circuits.
		} else {
			circuit_id_t id = ids.back();
			if (id == 0) {
				logError("Error", "Failed to load circuit file.");
				this->logError("Failed to load circuit.");
			} else {
				CircuitViewWidget& circuitViewWidget = createWidget<CircuitViewWidget>();
				circuitViewWidget.getCircuitView().setCircuit(id);
				for (auto& iter : environment.getBackend().getSimulatorManager().getSimulators()) {
					if (iter.second->getCircuitId() == id) {
						circuitViewWidget.getCircuitView().setSimulator(iter.first);
					}
				}
			}
		}
	} else if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) {
		applyUiScale(uiScale);
	} else {
		for (std::pair<const WidgetId, std::unique_ptr<Widget>>& widget : widgets) {
			widget.second->processEvent(event);
		}
	}
}

void MainWindow::render() {
	// global styling
	ImGuiStyle& style = ImGui::GetStyle();
	{std::lock_guard lock(uiScaleMux);
	style.FontScaleMain = uiScale;}
	style.TreeLinesFlags = ImGuiTreeNodeFlags_DrawLinesFull;
	style.TabCloseButtonMinWidthSelected = 0;
	style.TabRounding = 0;
	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10);
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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		if (ImGui::Begin("DockSpace", &open, window_flags)) {

			// setup window classes
			ImGuiWindowClass mainClass;
			mainClass.ClassId = 1;
			mainClass.DockingAllowUnclassed = false;
			ImGuiWindowClass sideBarClass;
			sideBarClass.ClassId = 2;
			sideBarClass.DockingAllowUnclassed = false;

			ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 5);
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::SetNextWindowSizeConstraints(ImVec2(40, 0), ImVec2(ImGui::GetContentRegionAvail().x - 40, FLT_MAX));
			ImGui::BeginChild("LeftRegion", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
			{
				dockLeftId = ImGui::GetID("LeftDock");
				if (ImGui::DockBuilderGetNode(dockLeftId) == NULL) {
					ImGui::DockBuilderRemoveNode(dockLeftId);
					ImGui::DockBuilderAddNode(
						dockLeftId,
						ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton
					);
				}
				ImGui::DockSpace(dockLeftId, ImVec2(0,0), 0, &sideBarClass);
				// kill split nodes with one window
				ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockMainId);
				if (node && node->IsSplitNode()) {
					ImGuiDockNode* child0 = node->ChildNodes[0];
					ImGuiDockNode* child1 = node->ChildNodes[1];
					if (child0 && child1) {
						if (child0->Windows.Size == 0)
							ImGui::DockBuilderRemoveNode(child0->ID);

						if (child1->Windows.Size == 0)
							ImGui::DockBuilderRemoveNode(child1->ID);
					}
				}
			}
			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::SetNextWindowSizeConstraints(ImVec2(40, 0), ImVec2(FLT_MAX, FLT_MAX));
			ImGui::BeginChild("MainRegion", ImVec2(0, 0), false);
			{
				dockMainId = ImGui::GetID("MainDock");
				if (ImGui::DockBuilderGetNode(dockMainId) == NULL) {
					ImGui::DockBuilderRemoveNode(dockMainId);
					ImGui::DockBuilderAddNode(
						dockMainId,
						ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton
					);
				}
				ImGui::DockSpace(dockMainId, ImVec2(0,0), 0, &mainClass);
				// kill split nodes with one window
				ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockLeftId);
				if (node && node->IsSplitNode()) {
					ImGuiDockNode* child0 = node->ChildNodes[0];
					ImGuiDockNode* child1 = node->ChildNodes[1];
					if (child0 && child1) {
						if (child0->Windows.Size == 0)
							ImGui::DockBuilderRemoveNode(child0->ID);

						if (child1->Windows.Size == 0)
							ImGui::DockBuilderRemoveNode(child1->ID);
					}
				}
			}
			ImGui::EndChild();
			ImGui::PopStyleVar(3);
		}
		ImGui::PopStyleVar(3);
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

		{
			std::lock_guard lock(widgetsMux);
			for (std::pair<const WidgetId, std::unique_ptr<Widget>>& widget : widgets) {
				widget.second->doRendering();
			}
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f));
		ImGui::RenderNotifications();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}
	ImGui::PopStyleVar();
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
