#include "circuitViewWidget.h"

#include "../mainWindow.h"
#include "SDL3/SDL_dialog.h"
#include "app.h"
#include "gpu/mainRenderer.h"
#include "gui/viewportManager/circuitView/circuitView.h"
#include "gui/viewportManager/circuitView/events/customEvents.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

void LoadCallback(void* userData, const char* const* filePaths, int filter) {
	CircuitViewWidget* circuitViewWidget = (CircuitViewWidget*)userData;
	if (filePaths && filePaths[0]) {
		std::string filePath = filePaths[0];
		std::vector<circuit_id_t> ids = circuitViewWidget->getMainWindow().getEnvironment().getCircuitFileManager().loadFromFile(filePath);
		logInfo("Requested load from '{}' produced {} circuit(s)", "CircuitViewWidget:LoadCallback", filePath, ids.size());
		if (ids.empty()) {
			logInfo("No circuits found in '{}'", "CircuitViewWidget:LoadCallback", filePath);
			// logError("Failed to load circuit file."); // Not a error! It is valid to load 0 circuits.
			return;
		}
		circuit_id_t id = ids.back();
		bool doSetCir = true;
		// circuitViewWidget->getCircuitView()->getBackend()->linkCircuitViewWithCircuit(circuitViewWidget->getCircuitView(), id);
		for (auto& iter : circuitViewWidget->getBackend().getSimulatorManager().getSimulators()) {
			if (iter.second->getCircuitId() == id) {
				doSetCir = false;
				circuitViewWidget->getCircuitView().setSimulator(iter.first);
				// circuitViewWidget->getCircuitView()->getBackend()->linkCircuitViewWithSimulator(circuitViewWidget->getCircuitView(), iter.first, Address());
				return;
			}
		}
		if (doSetCir) circuitViewWidget->getCircuitView().setCircuit(id);
		logInfo("Circuit view switched to loaded circuit {}", "CircuitViewWidget:LoadCallback", id);
	} else {
		logInfo("File dialog canceled.");
	}
}

CircuitViewWidget::CircuitViewWidget(WidgetId widgetId, MainWindow& mainWindow) : Widget(widgetId, mainWindow) {
	ViewportId viewportId = MainRenderer::get().registerViewport({ 100, 100 });
	circuitView = std::make_unique<CircuitView>(mainWindow.getEnvironment(), viewportId);
	getMainWindow().getToolManagerManager().addCircuitView(circuitView.get());
	circuit_id_t circuitId = mainWindow.getEnvironment().getBackend().createCircuit();
	std::optional<simulator_id_t> simulatorId = mainWindow.getEnvironment().getBackend().createSimulator(circuitId);
	circuitView->setSimulator(simulatorId.value(), Address());
	SharedCircuit circuit = mainWindow.getEnvironment().getBackend().getCircuit(circuitId);
	setupGUIValue<float>("AspectRatio", 1, [&](const float& aspectRatio) { circuitView->getViewManager().setAspectRatio(aspectRatio); });
	setupGUIValue<bool>("MouseLeftDown", false, [&](const bool& state) {
		if (state) {
			if (!valueOr(getGUIValue<bool>("MouseInView"), false)) return;
			// std::lock_guard lock(Settings::getSettingsMapReadLock()); // we will be running these on main thread
			if (getMainWindow().isPressingKeybind("Keybinds/Editing/Pick Block")) {
				std::unique_ptr<CircuitView>& circuitView = this->circuitView;
				Circuit* circuit = circuitView->getCircuit();
				if (circuit) {
					const Block* block = circuit->getBlockContainer().getBlock(circuitView->getViewManager().getPointerPosition().snap());
					if (block) {
						BlockType type = block->type();
						Orientation orientation = block->getOrientation();
						ToolManagerManager& toolManagerManager = this->getMainWindow().getToolManagerManager();
						toolManagerManager.setBlock(type);
						if (*Settings::get<SettingType::BOOL>("Preferences/Editing/Pick Block Copy Orientation")) {
							toolManagerManager.setOrientation(orientation);
						} else {
							toolManagerManager.setOrientation(Orientation());
						}
						return;
					}
				}
			}
			if (getMainWindow().isPressingKeybind("Keybinds/Camera/Pan")) {
				if (circuitView->getEventRegister().doEvent(PositionEvent("View Attach Anchor", circuitView->getViewManager().getPointerPosition()))) {
					return;
				}
			}
			circuitView->getEventRegister().doEvent(PositionEvent("Tool Primary Activate", circuitView->getViewManager().getPointerPosition()));
		} else {
			circuitView->getEventRegister().doEvent(PositionEvent("View Dettach Anchor", circuitView->getViewManager().getPointerPosition()));
			circuitView->getEventRegister().doEvent(PositionEvent("Tool Primary Deactivate", circuitView->getViewManager().getPointerPosition()));
		}
	});
	setupGUIValue<bool>("MouseRightDown", false, [&](const bool& state) {
		if (state) {
			if (!valueOr(getGUIValue<bool>("MouseInView"), false)) return;
			circuitView->getEventRegister().doEvent(PositionEvent("Tool Secondary Activate", circuitView->getViewManager().getPointerPosition()));
		} else {
			circuitView->getEventRegister().doEvent(PositionEvent("Tool Secondary Deactivate", circuitView->getViewManager().getPointerPosition()));
		}
	});
	setupGUIValue<Vec2>("MousePosition", {0, 0}, [&](const Vec2& viewPos) {
		circuitView->getEventRegister().doEvent(ViewPositionEvent("Pointer Move On View", viewPos));
	});
	setupGUIValue<bool>("MouseInView", false, [&](const bool& inView) {
		// const Vec2* viewPos = getGUIValue<Vec2>("MousePosition");
		// if (viewPos == nullptr) return;
		if (inView) {
			// if (viewPos->x < 0 || viewPos->y < 0 || viewPos->x > 1 || viewPos->y > 1) return;
			circuitView->getEventRegister().doEvent(PositionEvent("pointer enter view", circuitView->getViewManager().getPointerPosition()));
		} else {
			// if (viewPos->x >= 0 && viewPos->y >= 0 && viewPos->x <= 1 && viewPos->y <= 1) return;
			circuitView->getEventRegister().doEvent(PositionEvent("pointer exit view", circuitView->getViewManager().getPointerPosition()));
		}
	});
	setupGUIValue<Vec2>("CircuitViewSize", Vec2(1, 1), nullptr);
	setupGUIValue<bool>("CircuitViewIsHovered", false, nullptr);
	setupGUIValue<bool>("CircuitViewIsFocused", false, nullptr);
	setupGUIValue<float>("WindowScalingSize", getMainWindow().getWindowScalingSize(), nullptr);
	setupGUIValue<FPosition>("MouseGridPos", circuitView->getViewManager().getPointerPosition(), nullptr);
}

CircuitViewWidget::~CircuitViewWidget() {
	getMainWindow().getToolManagerManager().addCircuitView(circuitView.get());
	ViewportId viewportId = circuitView->getViewportId();
	circuitView.reset();
	MainRenderer::get().deregisterViewport(viewportId);
}

void CircuitViewWidget::processEvent(SDL_Event& event) {
	if (valueOr(getGUIValue<bool>("CircuitViewIsFocused"), false)) {

	}
	if (valueOr(getGUIValue<bool>("CircuitViewIsHovered"), false)) {
		if (event.type == SDL_EVENT_MOUSE_WHEEL) {
			Vec2 movement(-event.wheel.x * getMainWindow().getWindowScalingSize(), event.wheel.y * getMainWindow().getWindowScalingSize());
			if (!Settings::get<SettingType::BOOL>("Keybinds/Camera/Scroll Panning", false)) {
				circuitView->getEventRegister().doEvent(DeltaEvent("view zoom", (float)(movement.y) / 15.f));
			} else if (getMainWindow().isPressingKeybind("Keybinds/Camera/Zoom")) {
				// do zoom
				circuitView->getEventRegister().doEvent(DeltaEvent("view zoom", (float)(movement.y) / 15.f));
			} else {
				Vec2 size = valueOr(getGUIValue<Vec2>("CircuitViewSize"), Vec2(100, 100));
				float scaleAmout = App::getDetlaTime() * 1000.f;
				circuitView->getEventRegister().doEvent(DeltaXYEvent(
					"view pan",
					movement.x / size.x * circuitView->getViewManager().getViewWidth() * scaleAmout,
					movement.y / size.y * circuitView->getViewManager().getViewHeight() * scaleAmout
				));
			}
		} else if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != nullptr) {
			std::string filePath(event.drop.data);
			if (filePath.empty()) return;
			std::vector<circuit_id_t> ids = getEnvironment().getCircuitFileManager().loadFromFile(filePath);
			if (ids.empty()) {
				// logError("Error", "Failed to load circuit file."); // Not a error! It is valid to load 0 circuits.
			} else {
				circuit_id_t id = ids.back();
				if (id == 0) {
					logError("Error", "Failed to load circuit file.");
					getMainWindow().logError("Failed to load circuit.");
				} else {
					circuitView->setCircuit(id);
					for (auto& iter : circuitView->getBackend().getSimulatorManager().getSimulators()) {
						if (iter.second->getCircuitId() == id) {
							circuitView->setSimulator(iter.first);
						}
					}
				}
			}
		}
	}
}

circuit_id_t CircuitViewWidget::newCircuit() {
	circuit_id_t id = circuitView->getBackend().createCircuit();
	if (id == 0) return 0 ; // other logs shoud happen before this
	logInfo("Created new circuit {}", "CircuitViewWidget::newCircuit", id);
	circuitView->setCircuit(id);
	// tmp get eval with this circuit id because circuit manager makes a eval for loaded circuits
	for (auto& iter : circuitView->getBackend().getSimulatorManager().getSimulators()) {
		if (iter.second->getCircuitId() == id) {
			circuitView->setSimulator(iter.second.get());
			return id;
		}
	}
	return 0;
}

// save current circuit view widget we are viewing. Right now only works if it is the only widget in application.
// Called via Ctrl-S keybind
void CircuitViewWidget::save() {
	// if (circuitView->getCircuit()) mainWindow.getPopUpManager().savePopUp(circuitView->getCircuit()->getUUID());
	// else {
	// 	logWarning("Could not save because non circuit was selected.", "CircuitViewWidget");
	// 	mainWindow.log("Could not save because non circuit was selected.");
	// }
}

void CircuitViewWidget::asSave() {
	// if (circuitView->getCircuit()) mainWindow.getPopUpManager().saveAsPopUp(circuitView->getCircuit()->getUUID());
	// else {
	// 	logWarning("Could not save because non circuit was selected.", "CircuitViewWidget");
	// 	mainWindow.log("Could not save because non circuit was selected.");
	// }
}

// for drag and drop load directly onto this circuit view widget
void CircuitViewWidget::load() {
	static const SDL_DialogFileFilter filters[] = {
		{ "Circuit Files", "cir" },
		{ "BLIF Files", "blif" },
		{ "WASM Files", "wasm" },
	};

	logInfo("Opening load dialog for circuit import", "CircuitViewWidget::load");
	SDL_ShowOpenFileDialog(LoadCallback, this, nullptr, filters, 3, nullptr, true);
}

void CircuitViewWidget::render() {
	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
	ImGui::Begin(getWidgetIdStr().c_str());
	bool isHovered = false;
	bool isFocused = false;
	bool isActive = false;
	bool leftClick = false;
	bool rightClick = false;
	ImVec2 mousePos;
	if (ImGui::BeginChild("circuitView")) {
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImVec2 viewportWindowScreenPos = ImGui::GetCursorScreenPos();
		ImVec2 viewportWindowPos = ImGui::GetCursorPos();
		{
			float windowScalingSize = valueOr(getGUIValue_rendering<float>("WindowScalingSize"), 1.f);
			MainRenderer::get().resizeViewport(circuitView->getViewportId(), { viewportPanelSize.x * windowScalingSize, viewportPanelSize.y * windowScalingSize });
			VkDescriptorSet descriptorSet = MainRenderer::get().startViewportRendering(circuitView->getViewportId());
			if (descriptorSet != VK_NULL_HANDLE) {
				ImGui::Image(descriptorSet, ImVec2{ viewportPanelSize.x, viewportPanelSize.y });
			} else {
				ImGui::Text("RENDERING BROKEN!! :(");
			}
		}
		ImGui::SetCursorPos(viewportWindowPos);
		ImGui::InvisibleButton("circuitViewInvisibleButton", viewportPanelSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		isHovered = ImGui::IsItemHovered();
		if (isHovered) ImGui::FocusItem();
		isActive = ImGui::IsItemActive();
		isFocused = ImGui::IsItemFocused();
		mousePos = ImGui::GetMousePos();
		mousePos = ImVec2(mousePos.x - viewportWindowScreenPos.x, mousePos.y - viewportWindowScreenPos.y);

		setGUIValue_rendering("AspectRatio", viewportPanelSize.x / viewportPanelSize.y);
		setGUIValue_rendering("MouseLeftDown", ImGui::IsMouseDown(ImGuiMouseButton_Left));
		setGUIValue_rendering("MouseRightDown", ImGui::IsMouseDown(ImGuiMouseButton_Right));
		setGUIValue_rendering("MousePosition", Vec2(mousePos.x / viewportPanelSize.x, mousePos.y / viewportPanelSize.y));
		setGUIValue_rendering("MouseInView", isHovered);
		setGUIValue_rendering("CircuitViewSize", Vec2(viewportPanelSize.x, viewportPanelSize.y));
		setGUIValue_rendering("CircuitViewIsHovered", isHovered);
		setGUIValue_rendering("CircuitViewIsFocused", isFocused);

		ImGui::SetCursorPos(viewportWindowPos);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
		ImGui::BeginChild("circuitView", {0, 0}, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoInputs);
		{
			ImGui::PopStyleVar();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
			ImGui::Text("Mouse: %s", fmt::to_string(valueOr(getGUIValue_rendering<FPosition>("MouseGridPos"), FPosition::getInvalid())).c_str());
			// ImGui::Text("%s", fmt::format("isFocused: {}", isFocused).c_str());
			// ImGui::Text("%s", fmt::format("isActive: {}", isActive).c_str());
			// ImGui::Text("%s", fmt::format("mousePos: ({}, {})", mousePos.x, mousePos.y).c_str());
			// ImGui::Text("%s", fmt::format("ImGui {} fps", (int)ImGui::GetIO().Framerate).c_str());
			ImGui::PopStyleColor();
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
	ImGui::End();
}

void CircuitViewWidget::update() {
	if (valueOr(getGUIValue<bool>("CircuitViewIsFocused"), false)) {
		if (getMainWindow().isPressingKeybind("Keybinds/Camera/Home")) {
			 circuitView->getViewManager().focus(); }
		if (getMainWindow().isPressingKeybind("Keybinds/Editing/Undo")) {
			if (circuitView->getCircuit()) circuitView->getCircuit()->undo();
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Editing/Redo")) {
			if (circuitView->getCircuit()) circuitView->getCircuit()->redo();
		}
		if (getMainWindow().isPressingKeybind("Keybinds/File/Save")) {
			save();
		}
		if (getMainWindow().isPressingKeybind("Keybinds/File/Save As")) {
			asSave();
		}
		if (getMainWindow().isPressingKeybind("Keybinds/File/Open")) {
			load();
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Simulation/Start Stop")) {
			if (!circuitView->getSimulator()) {
				getMainWindow().logError("Cant start simulation when there is none");
				return;
			}
			circuitView->getSimulator()->setPause(!circuitView->getSimulator()->isPause());
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Simulation/Step Forward")) {
			if (!circuitView->getSimulator()) {
				getMainWindow().logError("Cant step simulation when there is none");
				return;
			}
			circuitView->getSimulator()->stepForward();
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Simulation/Step Back")) {
			if (!circuitView->getSimulator()) {
				getMainWindow().logError("Cant back step simulation when there is none");
				return;
			}
			if (circuitView->getSimulator()->stepBack()) {
			} else {
				getMainWindow().logError("Cant back step simulation with no simulation data");
			}
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Simulation/Increase Speed")) {
			if (!circuitView->getSimulator()) {
				getMainWindow().logError("Cant change simulation speed when there is none");
				return;
			}
			circuitView->getSimulator()->increaseTickrateSeq();
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Simulation/Decrease Speed")) {
			if (!circuitView->getSimulator()) {
				getMainWindow().logError("Cant change simulation speed when there is none");
				return;
			}
			circuitView->getSimulator()->decreaseTickrateSeq();
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Simulation/Skip Forward")) {
			if (circuitView->getSimulator()) {
				if (circuitView->getSimulator()->skipForward()) {
				} else {
					getMainWindow().logError("Cant skip forward simulation with no simulation data");
				}
			} else {
				getMainWindow().logError("Cant skip forward simulation when there is none");
			}
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Simulation/Skip Back")) {
			if (circuitView->getSimulator()) {
				if (circuitView->getSimulator()->skipBack()) {
				} else {
					getMainWindow().logError("Cant skip back simulation with no simulation data");
				}
			} else {
				getMainWindow().logError("Cant skip back simulation when there is none");
			}
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Simulation/Reset Simulation")) {
			if (!circuitView->getSimulator()) {
				getMainWindow().logError("Cant reset simulation when there is none");
				return;
			}
			bool showConfirm = *Settings::get<SettingType::BOOL>("Preferences/Simulation/Show Confirmation for Reset Simulation");
			if (showConfirm) {
				simulator_id_t simulatorId = circuitView->getSimulator()->getSimulatorId();
				EvalLogicSimulator* simulator = circuitView->getBackend().getSimulator(simulatorId);

				// getMainWindow().getPopUpManager().addOptionsPopUp(
				// 	"Reset Simulation States?",
				// 	{
				// 		{
				// 			"Reset",
				// 			[this, simulator]() {
				// 				simulator->resetStates();
				// 			}
				// 		},
				// 		{
				// 			"Cancel",
				// 			[]() {}
				// 		}
				// 	}
				// );
			} else {
				circuitView->getSimulator()->resetStates();
			}
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Editing/Copy")) {
			circuitView->getEventRegister().doEvent(Event("Copy"));
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Editing/Rotate CCW")) {
			circuitView->getEventRegister().doEvent(Event("Tool Rotate Block CCW"));
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Editing/Rotate CW")) {
			circuitView->getEventRegister().doEvent(Event("Tool Rotate Block CW"));
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Editing/Flip")) {
			circuitView->getEventRegister().doEvent(Event("Tool Flip Block"));
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Editing/Confirm")) {
			circuitView->getEventRegister().doEvent(Event("Confirm"));
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Editing/Tool Invert Mode")) {
			circuitView->getEventRegister().doEvent(Event("Tool Invert Mode"));
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Editing/Tools/Cycle Mode")) {
			getMainWindow().getToolManagerManager().cycleActiveToolMode();
		}
		if (getMainWindow().isPressingKeybind("Keybinds/Editing/Tools/Cycle Mode Back")) {
			getMainWindow().getToolManagerManager().cycleActiveToolMode(-1);
		}
		if (getMainWindow().isPressingKeybind("Keybinds/File/New")) {
			newCircuit();
		}
		// if (getMainWindow().isPressingKeybind("Keybinds/Tutorial/Start", [this]() { circuitView->getTutorialManager().StartTutorial(); });
		// if (getMainWindow().isPressingKeybind("Keybinds/Tutorial/Stop", [this]() { circuitView->getTutorialManager().Stop(); });
	}
}
