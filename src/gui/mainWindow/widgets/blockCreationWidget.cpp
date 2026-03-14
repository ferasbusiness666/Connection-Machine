#include "blockCreationWidget.h"

#include "gui/viewportManager/circuitView/tools/other/portAdder.h"
#include "gui/viewportManager/circuitView/events/customEvents.h"
#include "imgui/imgui_stdlib.h"

#include "gui/viewportManager/circuitView/circuitView.h"
#include "util/preprocessors.h"
#include "gui/mainWindow/mainWindow.h"
#include "gui/mainWindow/guiColors.h"
#include "app.h"

BlockCreationWidget::BlockCreationWidget(WidgetId widgetId, MainWindow& mainWindow, circuit_id_t circuitId) :
	Widget(widgetId, mainWindow), dataUpdateEventReceiver(getBackend().getDataUpdateEventManager()) {
	{
		ViewportId viewportId = MainRenderer::get().registerViewport(getMainWindow().getWindowId(), { 100, 100 });
		circuitView = std::make_unique<CircuitView>(mainWindow.getEnvironment(), viewportId);
		setupGUIValue<float>("AspectRatio", 1, [&](const float& aspectRatio) { circuitView->getViewManager().setAspectRatio(aspectRatio); });
		setupGUIValue<bool>("MouseLeftDown", false, [&](const bool& state) {
			if (state) {
				if (!getGUIValue<bool>("MouseInView")) return;
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
			if (inView) {
				circuitView->getEventRegister().doEvent(PositionEvent("pointer enter view", circuitView->getViewManager().getPointerPosition()));
			} else {
				circuitView->getEventRegister().doEvent(PositionEvent("pointer exit view", circuitView->getViewManager().getPointerPosition()));
			}
		});
		setupGUIValue<Vec2>("CircuitViewSize", Vec2(1, 1), nullptr);
		setupGUIValue<bool>("CircuitViewIsFocused", false, nullptr);
		setupGUIValue<float>("WindowScalingSize", getMainWindow().getWindowScalingSize(), nullptr);
		setupGUIValue<FPosition>("MouseGridPos", circuitView->getViewManager().getPointerPosition(), nullptr);
		setupGUIValue<std::string>("CircuitName", "NULL", nullptr);
	}
	{
		// =================================== Init rendering circuit ===================================
		renderingCircuitId = getBackend().getCircuitManager().createNewCircuit(true);
		assert(renderingCircuitId);
		Circuit* circuit = getBackend().getCircuitManager().getCircuit(renderingCircuitId).get();
		assert(circuit);
		circuit->setEditable(false);
		BlockData* blockData = getBackend().getBlockDataManager().getBlockData(circuit->getBlockType());
		assert(blockData);
		blockData->setIsPlaceable(false);
		circuitView->setCircuit(renderingCircuitId);
		for (auto& iter : circuitView->getBackend().getSimulatorManager().getSimulators()) {
			if (iter.second->getCircuitId() == renderingCircuitId) {
				circuitView->setSimulator(iter.second.get());
			}
		}
	}
	{
		// =================================== Init all data ===================================
		const CircuitBlockData* circuitBlockData = getBackend().getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
		if (circuitBlockData) {
			const BlockData* blockData = getBackend().getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
			assert(blockData);
			blockType = blockData->getBlockType();
			std::lock_guard mux(blockDataCopyMux);
			blockDataCopy = blockData->getBlockDataCopy();
		}
		{
			std::lock_guard mux(circuitsMux);
			for (const auto& circuit : getBackend().getCircuitManager().getCircuits()) {
				if (circuit.second->isEditable()) circuits.emplace(circuit.first, circuit.second->getCircuitName());
			}
		}
	}
	dataUpdateEventReceiver.linkFunction("blockDataUpdate", [this](const DataUpdateEventManager::EventData* event) {
		// update circuit list
		{
			std::lock_guard mux(circuitsMux);
			circuits.clear();
			for (const auto& circuit : getBackend().getCircuitManager().getCircuits()) {
				if (circuit.second->isEditable()) circuits.emplace(circuit.first, circuit.second->getCircuitName());
			}
		}
		// update renderData
		{
			const DataUpdateEventManager::EventDataWithValue<BlockType>* data = event->cast<BlockType>();
			assert(data);
			if (blockType != data->get()) return;
			const BlockData* blockData = getBackend().getBlockDataManager().getBlockData(blockType);
			assert(blockData);
			std::lock_guard mux(blockDataCopyMux);
			blockDataCopy = blockData->getBlockDataCopy();
		}
	});
	setupGUIValue<circuit_id_t>("circuitId", circuitId, [this](const circuit_id_t& circuitId) {
		const CircuitBlockData* circuitBlockData = getBackend().getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
		if (circuitBlockData == nullptr) {
			std::lock_guard mux(blockDataCopyMux);
			blockDataCopy = std::nullopt;
			circuitView->setCircuit(0);
			return;
		}
		const BlockData* blockData = getBackend().getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
		assert(blockData);
		blockType = blockData->getBlockType();
		std::lock_guard mux(blockDataCopyMux);
		blockDataCopy = blockData->getBlockDataCopy();
		Circuit* circuit = getBackend().getCircuit(renderingCircuitId).get();
		circuit->clear();
		circuit->tryInsertBlock(Position(), Orientation(), blockData->getBlockType());
		circuitView->getViewManager().focus();
	});
}

BlockCreationWidget::~BlockCreationWidget() {
	ViewportId viewportId = circuitView->getViewportId();
	circuitView.reset();
	MainRenderer::get().deregisterViewport(viewportId);
}

void BlockCreationWidget::processEvent(SDL_Event& event) {
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
				float scaleAmout = 5; //App::getDetlaTime() * 750.f;
				circuitView->getEventRegister().doEvent(DeltaXYEvent(
					"view pan",
					movement.x / size.x * circuitView->getViewManager().getViewWidth() * scaleAmout,
					movement.y / size.y * circuitView->getViewManager().getViewHeight() * scaleAmout
				));
			}
		}
	}
}

void BlockCreationWidget::render() {
	ImGui::SetNextWindowDockID(getMainWindow().getDockMainId(), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(100, 300), ImGuiCond_FirstUseEver);
	getMainWindow().setNextWindowMainDockable();
	bool open = true;
	getMainWindow().pushWindowStyling();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
	ifGui (ImGui::Begin(("Block Creation###" + getWidgetIdStr()).c_str(), &open),
		ImGui::PopStyleVar();
		getMainWindow().popWindowStyling();
	) {
		circuit_id_t circuitId = getGUIValue_rendering<circuit_id_t>("circuitId");
		auto iter = circuits.find(circuitId);
		std::string circuitName = "NONE";
		if (iter != circuits.end()) circuitName = iter->second;
		{ // circuit selector
			if (ImGui::BeginCombo("##circuitSelector", circuitName.c_str())) {
				static ImGuiTextFilter filter;
				if (ImGui::IsWindowAppearing()) {
					ImGui::SetKeyboardFocusHere();
					filter.Clear();
				}
				ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
				filter.Draw("##Filter", -FLT_MIN);

				for (const std::pair<circuit_id_t, std::string>& circuit : circuits)  {
					if (filter.PassFilter(circuit.second.c_str())) {
						if (ImGui::Selectable(circuit.second.c_str(), circuit.first == circuitId)) {
							setGUIValue_rendering("circuitId", circuit.first);
						}
					}
				}
				ImGui::EndCombo();
			}
		}
		std::lock_guard mux(blockDataCopyMux);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, GUIColors::BACKGROUND);
		ifGui (ImGui::BeginChild("##mainView", ImVec2(ImGui::GetContentRegionAvail().x - 200, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_ResizeX), ImGui::PopStyleColor()) {
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
			bool isHovered = ImGui::IsItemHovered();
			// if (isHovered) ImGui::FocusItem();
			ImVec2 mousePos = ImGui::GetMousePos();
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
					if (ImGui::Button("Add Port"))
						ImGui::OpenPopup("port_popup");

					if (blockType != BlockType::NONE) {
						ImGui::SameLine();
						ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.6f, 0.6f, 0.6f, 0.8f));
						ifGui (ImGui::BeginPopup("port_popup"), ImGui::PopStyleColor();) {
							if (ImGui::Selectable("Input")) {
								App::runOnMain([this, circuitId, name = blockDataCopy->name](){
									auto tool = std::dynamic_pointer_cast<PortAdder>(
										circuitView->getToolManager().selectTool(std::make_shared<PortAdder>(getEnvironment()))
									);
									if (tool) {
										tool->setup(true, [this](Position position){
											BlockData* blockData = getBackend().getBlockDataManager().getBlockData(blockType);
											if (blockData == nullptr) return;
											blockData->setConnectionInput(position - Position(), blockData->getNewConnectionId());
										});
									} else {
										logError("Failed to set tool to PortAdder", "BlockCreationWidget");
									}
								});
							}
							if (ImGui::Selectable("Output")) {
								App::runOnMain([this, circuitId, name = blockDataCopy->name](){
									auto tool = std::dynamic_pointer_cast<PortAdder>(
										circuitView->getToolManager().selectTool(std::make_shared<PortAdder>(getEnvironment()))
									);
									if (tool) {
										tool->setup(false, [this](Position position){
											BlockData* blockData = getBackend().getBlockDataManager().getBlockData(blockType);
											if (blockData == nullptr) return;
											blockData->setConnectionOutput(position - Position(), blockData->getNewConnectionId());
										});
									} else {
										logError("Failed to set tool to PortAdder", "BlockCreationWidget");
									}
								});
							}
							ImGui::EndPopup();
						}
					}
					ImGui::PopStyleColor();
				ImGui::EndGroup();
				simControlsSize = ImGui::GetItemRectSize();
			}
			{
				draw_list->ChannelsSetCurrent(0);
				if (blockType != BlockType::NONE) {
					ImGui::GetWindowDrawList()->AddRectFilled(
						{ viewportWindowScreenPos.x + offset, viewportWindowScreenPos.y + offset },
						{ viewportWindowScreenPos.x + simControlsSize.x + padding * 2 + offset, viewportWindowScreenPos.y + simControlsSize.y + padding * 2 + offset },
						ImColor(0.f, 0.f, 0.f, 0.1f),
						5.f
					);
				} else {
					ImGui::GetWindowDrawList()->AddRectFilled(
						{ viewportWindowScreenPos.x + offset, viewportWindowScreenPos.y + offset },
						{ viewportWindowScreenPos.x + simControlsSize.x + padding * 2 + offset, viewportWindowScreenPos.y + simControlsSize.y + padding * 2 + offset },
						ImColor(0.f, 0.f, 0.f, 0.2f),
						5.f
					);
				}
			}
			draw_list->ChannelsMerge();
		}
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_ChildBg, GUIColors::BACKGROUND);
		ifGui (ImGui::BeginChild("##SideBar"), ImGui::PopStyleColor()) {
			renderSideBar(circuitId);
		}
		ImGui::EndChild();
	}
	if (!open) {
		getMainWindow().destroyWidget(this->getWidgetId());
	}
	ImGui::End();
}

void BlockCreationWidget::renderSideBar(circuit_id_t circuitId) {
	// blockData
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, GUIColors::WIDGET_BACKGROUND);
	ifGui (ImGui::BeginChild("Block Data", ImVec2(ImGui::GetContentRegionAvail().x, 100), ImGuiChildFlags_ResizeY | ImGuiChildFlags_AlwaysUseWindowPadding),
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	) {
		{
			ImGui::Text("Name");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputText("##Name", &blockDataCopy->name)) {
				App::runOnMain([this, circuitId, name = blockDataCopy->name](){
					Backend& backend = getEnvironment().getBackend();
					Circuit* circuit = backend.getCircuit(circuitId).get();
					if (!circuit) return;
					circuit->setCircuitName(name);
				});
			}
		}
		{
			ImGui::Text("Size");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(min(ImGui::GetContentRegionAvail().x/2, 100));
			bool stateOffsetUpdated = ImGui::InputScalar("##xSize", ImGuiDataType_S32, &blockDataCopy->blockSize.w);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(min(ImGui::GetContentRegionAvail().x, 100));
			stateOffsetUpdated |= ImGui::InputScalar("##ySize", ImGuiDataType_S32, &blockDataCopy->blockSize.h);
			if (stateOffsetUpdated) {
				if (blockDataCopy->blockSize.w <= 0) blockDataCopy->blockSize.w = 1;
				if (blockDataCopy->blockSize.h <= 0) blockDataCopy->blockSize.h = 1;
				App::runOnMain([this, circuitId, size = blockDataCopy->blockSize](){
					Backend& backend = getMainWindow().getEnvironment().getBackend();
					const CircuitBlockData* circuitBlockData = backend.getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
					if (!circuitBlockData) return;
					BlockData* blockData = backend.getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
					assert(blockData);
					blockData->setSize(size);
				});
			}
		}
	}
	ImGui::EndChild();
	// renderDataList
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, GUIColors::WIDGET_BACKGROUND);
	ifGui (ImGui::BeginChild("Render Data", ImGui::GetContentRegionAvail(), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_MenuBar),
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("New")) {
				if (ImGui::MenuItem("Texture")) {
					App::runOnMain([this, circuitId](){
						Backend& backend = getMainWindow().getEnvironment().getBackend();
						const CircuitBlockData* circuitBlockData = backend.getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
						if (!circuitBlockData) return;
						BlockData* blockData = backend.getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
						assert(blockData);
						blockData->newRenderData<BlockData::BlockTextureData>();
					});
				}
				if (ImGui::MenuItem("Shape")) {
					getMainWindow().logError("Shape render data not real yet!");
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		for (unsigned int index = 0; index < blockDataCopy->renderData.size(); index++) {
			BlockData::RenderDataType& renderData = blockDataCopy->renderData[index];
			ImGui::PushID(index);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, (index % 2 ==0) ? GUIColors::WIDGET_ALTERNATING_BACKGROUND_1 : GUIColors::WIDGET_ALTERNATING_BACKGROUND_2);
			ifGui (ImGui::BeginChild("Render Data Row", ImVec2(ImGui::GetContentRegionAvail().x, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding),
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			) {
				if (std::holds_alternative<BlockData::BlockTextureData>(renderData)) {
					BlockData::BlockTextureData& blockTextureData = std::get<BlockData::BlockTextureData>(renderData);
					if (ImGui::TreeNodeEx("BlockTexture", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::TreePop();
						ImGui::Indent();
						if (ImGui::InputText("Path", &blockTextureData.path)) {
							App::runOnMain([this, circuitId, index, path = blockTextureData.path](){
								Backend& backend = getMainWindow().getEnvironment().getBackend();
								const CircuitBlockData* circuitBlockData = backend.getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
								if (!circuitBlockData) return;
								BlockData* blockData = backend.getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
								assert(blockData);
								if (!blockData->isRenderDataOfType<BlockData::BlockTextureData>(index)) return;
								blockData->setBlockTexturePath(index, path);
							});
						}
						if (ImGui::Checkbox("Use full texture", &blockTextureData.useFullTexture)) {
							App::runOnMain([this, circuitId, index, useFullTexture = blockTextureData.useFullTexture](){
								Backend& backend = getMainWindow().getEnvironment().getBackend();
								const CircuitBlockData* circuitBlockData = backend.getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
								if (!circuitBlockData) return;
								BlockData* blockData = backend.getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
								assert(blockData);
								if (!blockData->isRenderDataOfType<BlockData::BlockTextureData>(index)) return;
								blockData->setBlockUseFullTexture(index, useFullTexture);
							});
						}
						if (!blockTextureData.useFullTexture) {
							{
								ImGui::Text("Size");
								ImGui::SameLine();
								ImGui::SetNextItemWidth(min(ImGui::GetContentRegionAvail().x/2, 100));
								bool sizeUpdated = ImGui::InputScalar("##xSize", ImGuiDataType_U32, &blockTextureData.size.x);
								ImGui::SameLine();
								ImGui::SetNextItemWidth(min(ImGui::GetContentRegionAvail().x, 100));
								sizeUpdated |= ImGui::InputScalar("##ySize", ImGuiDataType_U32, &blockTextureData.size.y);
								if (sizeUpdated) {
									App::runOnMain([this, circuitId, index, size = blockTextureData.size](){
										Backend& backend = getMainWindow().getEnvironment().getBackend();
										const CircuitBlockData* circuitBlockData = backend.getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
										if (!circuitBlockData) return;
										BlockData* blockData = backend.getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
										assert(blockData);
										if (!blockData->isRenderDataOfType<BlockData::BlockTextureData>(index)) return;
										blockData->setBlockTextureSize(index, size);
									});
								}
							}
							{
								ImGui::Text("Top Left");
								ImGui::SameLine();
								ImGui::SetNextItemWidth(min(ImGui::GetContentRegionAvail().x/2, 100));
								bool topLeftUpdated = ImGui::InputScalar("##xTopLeft", ImGuiDataType_U32, &blockTextureData.topLeft.x);
								ImGui::SameLine();
								ImGui::SetNextItemWidth(min(ImGui::GetContentRegionAvail().x, 100));
								topLeftUpdated |= ImGui::InputScalar("##yTopLeft", ImGuiDataType_U32, &blockTextureData.topLeft.y);
								if (topLeftUpdated) {
									App::runOnMain([this, circuitId, index, topLeft = blockTextureData.topLeft](){
										Backend& backend = getMainWindow().getEnvironment().getBackend();
										const CircuitBlockData* circuitBlockData = backend.getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
										if (!circuitBlockData) return;
										BlockData* blockData = backend.getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
										assert(blockData);
										if (!blockData->isRenderDataOfType<BlockData::BlockTextureData>(index)) return;
										blockData->setBlockTextureTopLeft(index, topLeft);
									});
								}
							}
							if (ImGui::Checkbox("Display state", &blockTextureData.renderState)) {
								App::runOnMain([this, circuitId, index, renderState = blockTextureData.renderState](){
									Backend& backend = getMainWindow().getEnvironment().getBackend();
									const CircuitBlockData* circuitBlockData = backend.getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
									if (!circuitBlockData) return;
									BlockData* blockData = backend.getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
									assert(blockData);
									if (!blockData->isRenderDataOfType<BlockData::BlockTextureData>(index)) return;
									blockData->setBlockRenderState(index, renderState);
								});
							}
							if (blockTextureData.renderState) {
								{
									ImGui::SetNextItemWidth(min(ImGui::GetContentRegionAvail().x, 100));
									if (ImGui::InputScalar("Virtual connection id##virtutal", ImGuiDataType_S32, &blockTextureData.virtualConnectionId)) {
										App::runOnMain([this, circuitId, index, virtualConnectionId = blockTextureData.virtualConnectionId](){
											Backend& backend = getMainWindow().getEnvironment().getBackend();
											const CircuitBlockData* circuitBlockData = backend.getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
											if (!circuitBlockData) return;
											BlockData* blockData = backend.getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
											assert(blockData);
											if (!blockData->isRenderDataOfType<BlockData::BlockTextureData>(index)) return;
											blockData->setBlockTextureVirtualConnection(index, virtualConnectionId);
										});
									}
								}
								{
									ImGui::Text("State Offset");
									ImGui::SameLine();
									ImGui::SetNextItemWidth(min(ImGui::GetContentRegionAvail().x/2, 100));
									bool stateOffsetUpdated = ImGui::InputScalar("##xStateOffset", ImGuiDataType_S32, &blockTextureData.stateOffset.x);
									ImGui::SameLine();
									ImGui::SetNextItemWidth(min(ImGui::GetContentRegionAvail().x, 100));
									stateOffsetUpdated |= ImGui::InputScalar("##yStateOffset", ImGuiDataType_S32, &blockTextureData.stateOffset.y);
									if (stateOffsetUpdated) {
										App::runOnMain([this, circuitId, index, stateOffset = blockTextureData.stateOffset](){
											Backend& backend = getMainWindow().getEnvironment().getBackend();
											const CircuitBlockData* circuitBlockData = backend.getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
											if (!circuitBlockData) return;
											BlockData* blockData = backend.getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
											assert(blockData);
											if (!blockData->isRenderDataOfType<BlockData::BlockTextureData>(index)) return;
											blockData->setBlockTextureStateOffset(index, stateOffset);
										});
									}
								}
							}
						}
						ImGui::Unindent();
					}
				}
			}
			ImGui::EndChild();
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
}

void BlockCreationWidget::update() {
	if (circuitView->getCircuit()) {
		setGUIValue<std::string>("CircuitName", circuitView->getCircuit()->getCircuitName());
	} else {
		setGUIValue<std::string>("CircuitName", "NULL");
	}
	setGUIValue<FPosition>("MouseGridPos", circuitView->getViewManager().getPointerPosition());
	if (getGUIValue<bool>("CircuitViewIsFocused")) {
		if (getMainWindow().isPressingKeybind("Keybinds/Camera/Home")) {
			 circuitView->getViewManager().focus(); }
	}
}
