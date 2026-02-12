#include "circuitViewWidget.h"

#include "../mainWindow.h"
#include "SDL3/SDL_dialog.h"
#include "app.h"
#include "gpu/mainRenderer.h"
#include "gui/viewportManager/circuitView/circuitView.h"
#include "gui/viewportManager/circuitView/events/customEvents.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

CircuitViewWidget::CircuitViewWidget(WidgetId widgetId, MainWindow& mainWindow) : Widget(widgetId, mainWindow) {
	ViewportId viewportId = MainRenderer::get().registerViewport(getMainWindow().getWindowId(), { 100, 100 });
	circuitView = std::make_unique<CircuitView>(mainWindow.getEnvironment(), viewportId);
	getMainWindow().getToolManagerManager().addCircuitView(circuitView.get());
	setupGUIValue<float>("AspectRatio", 1, [&](const float& aspectRatio) { circuitView->getViewManager().setAspectRatio(aspectRatio); });
	setupGUIValue<bool>("MouseLeftDown", false, [&](const bool& state) {
		if (state) {
			if (!getGUIValue<bool>("MouseInView")) return;
			// std::lock_guard lock(Settings::getSettingsMapReadLock()); // we will be running these on main thread
			if (getMainWindow().isPressingKeybind("Keybinds/Editing/Pick Block", true)) {
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
			if (getMainWindow().isPressingKeybind("Keybinds/Camera/Pan", true)) {
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
			if (!getGUIValue<bool>("MouseInView")) return;
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
	setupGUIValue<bool>("CircuitViewIsFocused", false, nullptr);
	setupGUIValue<float>("WindowScalingSize", getMainWindow().getWindowScalingSize(), nullptr);
	setupGUIValue<FPosition>("MouseGridPos", circuitView->getViewManager().getPointerPosition(), nullptr);
	setupGUIValue<double>("SimulatorRealTPS", 0, nullptr);
	setupGUIValue<double>("SimulatorTargetTPS", 40, [this](const double& tps) {
		if (circuitView->getSimulator()) {
			circuitView->getSimulator()->setTickrate(tps);
		}
	});
	setupGUIValue<bool>("SimulatorLimitSpeed", true, [this](const bool& doLimit) {
		if (circuitView->getSimulator()) {
			circuitView->getSimulator()->setUseTickrate(doLimit);
		}
	});
	setupGUIValue<bool>("SimulatorPaused", true, [this](const bool& isPaused) {
		if (circuitView->getSimulator()) {
			circuitView->getSimulator()->setPause(isPaused);
		}
	});
	setupGUIValue<std::string>("CircuitName", "NULL", nullptr);
	setupGUIValue<bool>("CircuitSaved", false, nullptr);
}

CircuitViewWidget::~CircuitViewWidget() {
	getMainWindow().getToolManagerManager().removeCircuitView(circuitView.get());
	ViewportId viewportId = circuitView->getViewportId();
	circuitView.reset();
	MainRenderer::get().deregisterViewport(viewportId);
}

void CircuitViewWidget::processEvent(SDL_Event& event) {
	if (getGUIValue<bool>("CircuitViewIsFocused")) {

	}
	if (getGUIValue<bool>("MouseInView")) {
		if (event.type == SDL_EVENT_MOUSE_WHEEL) {
			Vec2 movement(-event.wheel.x * getMainWindow().getWindowScalingSize(), event.wheel.y * getMainWindow().getWindowScalingSize());
			if (!Settings::get<SettingType::BOOL>("Keybinds/Camera/Scroll Panning", false)) {
				circuitView->getEventRegister().doEvent(DeltaEvent("view zoom", (float)(movement.y) / 15.f));
			} else if (getMainWindow().isPressingKeybind("Keybinds/Camera/Zoom", true)) {
				// do zoom
				circuitView->getEventRegister().doEvent(DeltaEvent("view zoom", (float)(movement.y) / 15.f));
			} else {
				Vec2 size = getGUIValue<Vec2>("CircuitViewSize");
				float scaleAmout = App::getDetlaTime() * 750.f;
				circuitView->getEventRegister().doEvent(DeltaXYEvent(
					"view pan",
					movement.x / size.x * circuitView->getViewManager().getViewWidth() * scaleAmout,
					movement.y / size.y * circuitView->getViewManager().getViewHeight() * scaleAmout
				));
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
	if (circuitView->getCircuit()) {
		if (getMainWindow().getEnvironment().getCircuitFileManager().save(circuitView->getCircuit()->getUUID())) {
			getMainWindow().log("Saved circuit {}", circuitView->getCircuit()->getCircuitName());
		} else {
			getMainWindow().logError("Failed to save circuit {}.", circuitView->getCircuit()->getCircuitName());
		}
	} else {
		logWarning("Could not save because non circuit was selected.", "CircuitViewWidget");
		getMainWindow().log("Could not save because non circuit was selected.");
	}
}

void CircuitViewWidget::asSave() {
	getMainWindow().log("Not implemented.");
	// if (circuitView->getCircuit()) getMainWindow().getPopUpManager().saveAsPopUp(circuitView->getCircuit()->getUUID());
	// else {
	// 	logWarning("Could not save because non circuit was selected.", "CircuitViewWidget");
	// 	getMainWindow().log("Could not save because non circuit was selected.");
	// }
}

void CircuitViewWidget::render() {
	ImGui::SetNextWindowDockID(getMainWindow().getDockMainId(), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
	getMainWindow().setNextWindowMainDockable();
	bool isHovered = false;
	bool isFocused = false;
	ImVec2 mousePos;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
	if (!getGUIValue_rendering<bool>("CircuitSaved")) {
		windowFlags |= ImGuiWindowFlags_UnsavedDocument;
	}
	bool open = true;
	if (ImGui::Begin((getGUIValue_rendering<std::string>("CircuitName") + "###" + getWidgetIdStr()).c_str(), &open, windowFlags)) {
		ImGui::PopStyleVar();
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImVec2 viewportWindowScreenPos = ImGui::GetCursorScreenPos();
		ImVec2 viewportWindowPos = ImGui::GetCursorPos();
		{
			float windowScalingSize = getGUIValue_rendering<float>("WindowScalingSize");
			MainRenderer::get().resizeViewport(circuitView->getViewportId(), { viewportPanelSize.x * windowScalingSize, viewportPanelSize.y * windowScalingSize });
			VkDescriptorSet descriptorSet = MainRenderer::get().startViewportRendering(circuitView->getViewportId());
			if (descriptorSet != VK_NULL_HANDLE) {
				ImGui::Image(descriptorSet, ImVec2{ viewportPanelSize.x, viewportPanelSize.y });
			} else {
				ImGui::Text("RENDERING BROKEN!! :(");
			}
		}
		ImGui::SetCursorPos(viewportWindowPos);
		ImGui::SetNextItemAllowOverlap();
		ImGui::InvisibleButton("circuitViewInvisibleButton", viewportPanelSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		isHovered = ImGui::IsItemHovered();
		if (isHovered) ImGui::FocusItem();
		mousePos = ImGui::GetMousePos();
		mousePos = ImVec2(mousePos.x - viewportWindowScreenPos.x, mousePos.y - viewportWindowScreenPos.y);

		setGUIValue_rendering("AspectRatio", viewportPanelSize.x / viewportPanelSize.y);
		setGUIValue_rendering("MouseLeftDown", ImGui::IsMouseDown(ImGuiMouseButton_Left));
		setGUIValue_rendering("MouseRightDown", ImGui::IsMouseDown(ImGuiMouseButton_Right));
		setGUIValue_rendering("MousePosition", Vec2(mousePos.x / viewportPanelSize.x, mousePos.y / viewportPanelSize.y));
		setGUIValue_rendering("MouseInView", isHovered);
		setGUIValue_rendering("CircuitViewSize", Vec2(viewportPanelSize.x, viewportPanelSize.y));
		setGUIValue_rendering("CircuitViewIsFocused", ImGui::IsItemFocused());

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->ChannelsSplit(2);
		ImVec2 simControlsSize;
		const int offset = 2;
		const int padding = 4;
		{
			draw_list->ChannelsSetCurrent(1);
			ImGui::SetCursorPos({ viewportWindowPos.x + padding + offset, viewportWindowPos.y + padding + offset });
			ImGui::BeginGroup();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
				FPosition mouseGridPos = getGUIValue_rendering<FPosition>("MouseGridPos");
				ImGui::Text("Mouse: (%.2f, %.2f)", mouseGridPos.x, mouseGridPos.y);
				ImGui::Text("RTPS: %.2f", getGUIValue_rendering<double>("SimulatorRealTPS"));
				ImGui::SameLine();
				ImGui::PushItemWidth(50);
				ImGui::InputDouble("TTPS", getGUIValueForImGui_rendering<double>("SimulatorTargetTPS"), 0, 0, "%.2f");
				ImGui::PopItemWidth();
				ImGui::Checkbox("Paused", getGUIValueForImGui_rendering<bool>("SimulatorPaused"));
				ImGui::SameLine();
				ImGui::Checkbox("Limit Speed", getGUIValueForImGui_rendering<bool>("SimulatorLimitSpeed"));
				ImGui::PopStyleColor();
			ImGui::EndGroup();
			simControlsSize = ImGui::GetItemRectSize();
		}
		{
			draw_list->ChannelsSetCurrent(0);
			ImGui::GetWindowDrawList()->AddRectFilled(
				{ viewportWindowScreenPos.x + offset, viewportWindowScreenPos.y + offset },
				{ viewportWindowScreenPos.x + simControlsSize.x + padding * 2 + offset, viewportWindowScreenPos.y + simControlsSize.y + padding * 2 + offset },
				ImColor(0.f, 0.f, 0.f, 0.2f),
				5.f
			);
		}
		draw_list->ChannelsMerge();
	} else ImGui::PopStyleVar();
	if (!open) {
		getMainWindow().destroyWidget(this->getWidgetId());
	}
	ImGui::End();
}

void CircuitViewWidget::update() {
	if (circuitView->getCircuit()) {
		setGUIValue<std::string>("CircuitName", circuitView->getCircuit()->getCircuitName());
		setGUIValue<bool>("CircuitSaved", getMainWindow().getEnvironment().getCircuitFileManager().isCircuitSaved(circuitView->getCircuit()->getUUID()));
	} else {
		setGUIValue<bool>("CircuitSaved", true);
		setGUIValue<std::string>("CircuitName", "NULL");
	}
	setGUIValue<FPosition>("MouseGridPos", circuitView->getViewManager().getPointerPosition());
	setGUIValue<double>("SimulatorRealTPS", circuitView->getSimulator() ? circuitView->getSimulator()->getRealTickrate() : 0);
	setGUIValue<double>("SimulatorTargetTPS", circuitView->getSimulator() ? circuitView->getSimulator()->getTickrate() : 0);
	setGUIValue<bool>("SimulatorLimitSpeed", circuitView->getSimulator() ? circuitView->getSimulator()->getUseTickrate() : true);
	setGUIValue<bool>("SimulatorPaused", circuitView->getSimulator() ? circuitView->getSimulator()->isPause() : true);
	if (getGUIValue<bool>("CircuitViewIsFocused")) {
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
