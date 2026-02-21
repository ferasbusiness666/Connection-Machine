#include "blockCreationWidget.h"

#include "../mainWindow.h"

#include "app.h"
#include "gui/mainWindow/widgets/circuitViewWidget.h"
#include "imgui/imgui_stdlib.h"
#include "../guiColors.h"

BlockCreationWidget::BlockCreationWidget(WidgetId widgetId, MainWindow& mainWindow, circuit_id_t circuitId) :
	Widget(widgetId, mainWindow), dataUpdateEventReceiver(getBackend().getDataUpdateEventManager()) {
	{
		// =================================== Init all data ===================================
		const CircuitBlockData* circuitBlockData = getMainWindow().getEnvironment().getBackend().getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
		if (circuitBlockData) {
			const BlockData* blockData = getMainWindow().getEnvironment().getBackend().getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
			assert(blockData);
			blockType = blockData->getBlockType();
			std::lock_guard mux(renderDataRowsMux);
			renderDataRows = blockData->getRenderData();
		}
		{
			std::lock_guard mux(circuitsMux);
			for (const auto& circuit : getMainWindow().getEnvironment().getBackend().getCircuitManager().getCircuits()) {
				circuits.emplace(circuit.first, circuit.second->getCircuitName());
			}
		}
	}
	dataUpdateEventReceiver.linkFunction("blockDataUpdate", [this](const DataUpdateEventManager::EventData* event) {
		// update circuit list
		{
			std::lock_guard mux(circuitsMux);
			for (const auto& circuit : getMainWindow().getEnvironment().getBackend().getCircuitManager().getCircuits()) {
				circuits.emplace(circuit.first, circuit.second->getCircuitName());
			}
		}
		// update renderData
		{
			const DataUpdateEventManager::EventDataWithValue<BlockType>* data = event->cast<BlockType>();
			assert(data);
			if (blockType != data->get()) return;
			const BlockData* blockData = getBackend().getBlockDataManager().getBlockData(blockType);
			assert(blockData);
			std::lock_guard mux(renderDataRowsMux);
			renderDataRows = blockData->getRenderData();
		}
	});
	setupGUIValue<circuit_id_t>("circuitId", circuitId, [this](const circuit_id_t& circuitId) {
		const CircuitBlockData* circuitBlockData = getMainWindow().getEnvironment().getBackend().getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
		if (circuitBlockData == nullptr) {
			std::lock_guard mux(renderDataRowsMux);
			renderDataRows.clear();
			return;
		}
		const BlockData* blockData = getBackend().getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
		assert(blockData);
		blockType = blockData->getBlockType();
		std::lock_guard mux(renderDataRowsMux);
		renderDataRows = blockData->getRenderData();
	});
}

void BlockCreationWidget::render() {
	ImGui::SetNextWindowDockID(getMainWindow().getDockMainId(), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(100, 300), ImGuiCond_FirstUseEver);
	getMainWindow().setNextWindowMainDockable();
	bool open = true;
	getMainWindow().pushWindowStyling();
	if (ImGui::Begin(("Block Creation###" + getWidgetIdStr()).c_str(), &open)) {
		getMainWindow().popWindowStyling();
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
		{ // renderDataList
			ImGui::PushStyleColor(ImGuiCol_ChildBg, GUIColors::WIDGET_BACKGROUND);
			if (ImGui::BeginChild("Render Data", ImVec2(ImGui::GetContentRegionAvail().x, 400), ImGuiChildFlags_ResizeY, ImGuiWindowFlags_MenuBar)) {
				ImGui::PopStyleColor();
				{
					if (ImGui::BeginMenuBar()) {
						if (ImGui::BeginMenu("Add")) {
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

					std::lock_guard mux(renderDataRowsMux);

					for (unsigned int index = 0; index < renderDataRows.size(); index++) {
						BlockData::RenderDataType& renderData = renderDataRows[index];
						if (std::holds_alternative<BlockData::BlockTextureData>(renderData)) {
							BlockData::BlockTextureData& blockTextureData = std::get<BlockData::BlockTextureData>(renderData);
							ImGui::PushID(index);
							if (ImGui::TreeNodeEx("BlockTexture", ImGuiTreeNodeFlags_DefaultOpen)) {
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
								ImGui::TreePop();
							}
							ImGui::PopID();
						}
					}
				}
				ImGui::EndChild();
			} else ImGui::PopStyleColor();
		}
	} else getMainWindow().popWindowStyling();
	if (!open) {
		getMainWindow().destroyWidget(this->getWidgetId());
	}
	ImGui::End();
}
