#include "circuitViewWidget.h"

#include "../mainWindow.h"
#include "app.h"
#include "gpu/mainRenderer.h"
#include "gui/viewportManager/circuitView/circuitView.h"
#include "gui/viewportManager/circuitView/events/customEvents.h"
#include "imgui/imgui.h"

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
			if (getMainWindow().isPressingKeybind(*Settings::get<SettingType::KEYBIND>("Keybinds/Editing/Pick Block"))) {
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
			if (getMainWindow().isPressingKeybind(*Settings::get<SettingType::KEYBIND>("Keybinds/Camera/Pan"))) {
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
			const bool* scrollPanning = Settings::get<SettingType::BOOL>("Keybinds/Camera/Scroll Panning");
			if (scrollPanning && !*scrollPanning) {
				circuitView->getEventRegister().doEvent(DeltaEvent("view zoom", (float)(movement.y) / 15.f));
				return;
			}
			const Keybind* keybind = Settings::get<SettingType::KEYBIND>("Keybinds/Camera/Zoom");
			if (keybind == nullptr) return;
			if (getMainWindow().isPressingKeybind(*keybind)) {
				// do zoom
				circuitView->getEventRegister().doEvent(DeltaEvent("view zoom", (float)(movement.y) / 15.f));
			} else {
				Vec2 size = valueOr(getGUIValue<Vec2>("CircuitViewSize"), Vec2(100, 100));
				float scaleAmout = App::getDetlaTime() * 220.;
				circuitView->getEventRegister().doEvent(DeltaXYEvent(
					"view pan",
					movement.x / size.x * circuitView->getViewManager().getViewWidth() * scaleAmout,
					movement.y / size.y * circuitView->getViewManager().getViewHeight() * scaleAmout
				));
			}
		} else if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != nullptr) {
			std::string filePath(event.drop.data);
			if (filePath.empty()) return;
			std::vector<circuit_id_t> ids = getMainWindow().getEnvironment().getCircuitFileManager().loadFromFile(filePath);
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
		isActive = ImGui::IsItemActive();
		isFocused = ImGui::IsItemFocused();
		isHovered = ImGui::IsItemHovered();
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
			ImGui::Text("%s", fmt::format("isHovered: {}", isHovered).c_str());
			ImGui::Text("%s", fmt::format("isFocused: {}", isFocused).c_str());
			ImGui::Text("%s", fmt::format("isActive: {}", isActive).c_str());
			ImGui::Text("%s", fmt::format("mousePos: ({}, {})", mousePos.x, mousePos.y).c_str());
			ImGui::Text("%s", fmt::format("ImGui {} fps", (int)ImGui::GetIO().Framerate).c_str());
			ImGui::PopStyleColor();
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
	ImGui::End();
}
