#include "circuitTestWidget.h"

#include "app.h"
#include "backend/circuit/circuitDefs.h"
#include "backend/circuitTests/circuitTestGroup.h"
#include "backend/container/block/block.h"
#include "backend/container/block/blockDefs.h"
#include "gui/viewportManager/circuitView/events/customEvents.h"
#include "imgui/imgui.h"
#include "gui/viewportManager/circuitView/tools/other/treeTraversal.h"
#include "imgui/imgui_internal.h"

#include "gui/viewportManager/circuitView/circuitView.h"
#include "util/preprocessors.h"
#include "gui/mainWindow/mainWindow.h"
#include "gui/mainWindow/guiColors.h"
#include <mutex>
#include <string>

CircuitTestWidget::CircuitTestWidget(WidgetId widgetId, MainWindow& mainWindow) :
	Widget(widgetId, mainWindow), dataUpdateEventReceiver(getBackend().getDataUpdateEventManager()) {
	{
		ViewportId viewportId = MainRenderer::get().registerViewport(getMainWindow().getWindowId(), { 100, 100 });
		circuitView = std::make_unique<CircuitView>(mainWindow.getEnvironment(), viewportId);
		setupGUIValue<float>("AspectRatio", 1, [&](const float& aspectRatio) { circuitView->getViewManager().setAspectRatio(aspectRatio); });
		setupGUIValue<bool>("MouseLeftDown", false, [&](const bool& state) { //sets up a value for use with getGUIValue
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
		setupGUIValue<std::string>("StatusBar", "", nullptr);
		circuitView->getEventRegister().registerFunction("status bar changed", [this](const Event* event) -> bool {
			const EventWithValue<std::string>* data = event->cast2<std::string>();
			if (!data) return false;
			setGUIValue<std::string>("StatusBar", data->get());
			return true;
		});
		setupGUIValue<FPosition>("ViewportCenter", circuitView->getViewManager().getViewCenter(), nullptr);
		setupGUIValue<FVector>("ViewportCellSize", FVector(circuitView->getViewManager().getViewHeight(), circuitView->getViewManager().getViewWidth()), nullptr);
		circuitView->getEventRegister().registerFunction("ViewCenterMove", [this](const Event* event) -> bool {
			const PositionEvent* positionEvent = event->cast<PositionEvent>();
			if (positionEvent) setGUIValue<FPosition>("ViewportCenter", positionEvent->getFPosition());
			return false;
		});
		circuitView->getEventRegister().registerFunction("ViewSizeChange", [this](const Event* event) -> bool {
			const VectorEvent* vectorEvent = event->cast<VectorEvent>();
			if (vectorEvent) setGUIValue<FVector>("ViewportCellSize", vectorEvent->getFVector());
			return false;
		});
	}
	// {
	// 	// =================================== Init rendering circuit ===================================
	// 	std::lock_guard renderingCircuitMux(this->renderingCircuitMux);
	// 	renderingCircuitId = getBackend().getCircuitManager().createNewCircuit(true);
	// 	assert(renderingCircuitId);
	// 	Circuit* circuit = getBackend().getCircuitManager().getCircuit(renderingCircuitId);
	// 	assert(circuit);
	// 	circuit->setEditable(false);
	// 	BlockData* blockData = getBackend().getBlockDataManager().getBlockData(circuit->getBlockType());
	// 	assert(blockData);
	// 	blockData->setIsPlaceable(false);
	// 	circuitView->setCircuit(renderingCircuitId);
	// 	for (auto& iter : circuitView->getBackend().getSimulatorManager().getSimulators()) {
	// 		if (iter.second->getCircuitId() == renderingCircuitId) {
	// 			circuitView->setSimulator(iter.second.get());
	// 		}
	// 	}
	// }
    {
        std::lock_guard mux(blockTypesMux);
		for (unsigned int type = 0; type < getBackend().getBlockDataManager().maxBlockId(); type++) {
			const BlockData* blockData = getBackend().getBlockDataManager().getBlockData((BlockType)(type + 1));
			if (blockData) {
				if (blockData->isBus() || !blockData->isPlaceable()) continue;
				blockTypes.emplace(blockData->getBlockType(), blockData->getName());
			}
		}
	}
	{
		std::lock_guard mux(testGroupsMux);
		for (const auto& circuitTestGroup : getBackend().getCircuitTestGroupManager()) {
			testGroups.push_back(circuitTestGroup.first);
		}
    }
	dataUpdateEventReceiver.linkFunction("testGroupUpdate", [this](const DataUpdateEventManager::EventData* event) {
		const DataUpdateEventManager::EventDataWithValue<std::string>* passedName = event->cast<std::string>();
		assert(passedName);
		if (passedName->get() != getGUIValue<std::string>("testGroupName")) return;
		const CircuitTestGroup* circuitTestGroup = getBackend().getCircuitTestGroupManager().getCircuitTestGroup(getGUIValue<std::string>("testGroupName"));
		if (!circuitTestGroup) {
			setGUIValue<std::string>("testGroupName", "NONE");
			return;
		}
		std::lock_guard mux(testGroupCopyMux);
		testGroupCopy = circuitTestGroup->getMinimalCopy();
	});
	dataUpdateEventReceiver.linkFunction("newTestGroup", [this](const DataUpdateEventManager::EventData* event) {
		std::lock_guard mux(testGroupsMux);
		testGroups.clear();
		for (const auto& circuitTestGroup : getBackend().getCircuitTestGroupManager()) {
			testGroups.push_back(circuitTestGroup.first);
		}
	});
	/*
	dataUpdateEventReceiver.linkFunction("testRunDataUpdate", [this](const DataUpdateEventManager::EventData* event) {
		// make recieving test results a thing
		const DataUpdateEventManager::EventDataWithValue<std::string>*  = event->cast<std::tuple<std::string, circuit_id_t, int>>();
		const auto& [recievedName, recievedCircuitID, recievedIndex] =
		assert(passedName);
		if (passedName->get() != getGUIValue<std::string>("testGroupName")) return;
		const CircuitTestGroup* circuitTestGroup = getBackend().getCircuitTestGroupManager().getCircuitTestGroup(getGUIValue<std::string>("testGroupName"));
		if (!circuitTestGroup) {
			setGUIValue<std::string>("testGroupName", "NONE");
			return;
		}
		std::lock_guard mux(testGroupCopyMux);
		testGroupCopy = circuitTestGroup->getMinimalCopy();
	});*/
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
	setupGUIValue<BlockType>("blockType", BlockType::NONE, [this](const BlockType& blockType) {
		if (getGUIValue<std::string>("testGroupName") != "NONE") {
			testGroupRunner.emplace(getBackend(), getGUIValue<std::string>("testGroupName"), blockType);
			std::lock_guard renderingCircuitMux(this->renderingCircuitMux);
			renderingCircuitId = testGroupRunner.value().getCircuitId();
			circuitView->setSimulator(testGroupRunner->getSimulatorId());
		}
		setGUIValue("blockType", blockType);
		// const CircuitBlockData* circuitBlockData = getBackend().getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
		// if (circuitBlockData == nullptr) {
		// 	// std::lock_guard mux(blockDataCopyMux);
		// 	// blockDataCopy = std::nullopt;
		// 	circuitView->setCircuit(0);
        //     blockType = NONE;
		// 	return;
		// }
		// const BlockData* blockData = getBackend().getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
		// assert(blockData);
		// blockType = blockData->getBlockType();
		// std::lock_guard mux(blockDataCopyMux);
		// blockDataCopy = blockData->getBlockDataCopy();
		// Circuit* circuit = getBackend().getCircuit(renderingCircuitId).get();
		// circuit->clear();
		// circuit->tryInsertBlock(Position(), Orientation(), blockData->getBlockType());
		// circuitView->getViewManager().focus();
		// setGUIValue<std::optional<connection_end_id_t>>("currentlyEditingPort", std::nullopt);
	});
	setupGUIValue<std::string>("testGroupName", "NONE", [this](const std::string& testGroupName) {
		const CircuitTestGroup* circuitTestGroup = getBackend().getCircuitTestGroupManager().getCircuitTestGroup(testGroupName);
		if (!circuitTestGroup) {
			setGUIValue<std::string>("testGroupName", "NONE");
			testGroupRunner.reset();
			std::lock_guard renderingCircuitMux(this->renderingCircuitMux);
			renderingCircuitId = 0;
			circuitView->setSimulator(0);
			return;
		}

		{
			std::lock_guard mux(testGroupCopyMux);
			testGroupCopy = circuitTestGroup->getMinimalCopy();
		}
		setGUIValue<std::string>("testGroupName", testGroupName);
		testGroupRunner.emplace(getBackend(), getGUIValue<std::string>("testGroupName"), getGUIValue<BlockType>("blockType"));
		std::lock_guard renderingCircuitMux(this->renderingCircuitMux);
		renderingCircuitId = testGroupRunner.value().getCircuitId();
		circuitView->setSimulator(testGroupRunner->getSimulatorId());
	});
}

CircuitTestWidget::~CircuitTestWidget() {
	ViewportId viewportId = circuitView->getViewportId();
	circuitView.reset();
	MainRenderer::get().deregisterViewport(viewportId);
}

void CircuitTestWidget::processEvent(SDL_Event& event) {
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

void CircuitTestWidget::render() {
	ImGui::SetNextWindowDockID(getMainWindow().getDockMainId(), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(100, 300), ImGuiCond_FirstUseEver);
	getMainWindow().setNextWindowMainDockable();
	bool open = true;
	getMainWindow().pushWindowStyling();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
	ifGui (ImGui::Begin(("Circuit Test###" + getWidgetIdStr()).c_str(), &open),
		ImGui::PopStyleVar();
		getMainWindow().popWindowStyling();
	) {
		std::string testGroupName = getGUIValue_rendering<std::string>("testGroupName");
		{
			std::lock_guard mux(testGroupsMux);
			{
				if (ImGui::BeginCombo("##testGroupSelector", testGroupName.c_str())) {
					static ImGuiTextFilter filter;
					if (ImGui::IsWindowAppearing()) {
						ImGui::SetKeyboardFocusHere();
						filter.Clear();
					}
					ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
					filter.Draw("##Filter", -FLT_MIN);

					for (const std::string& testGroup : testGroups)  {
						if (filter.PassFilter(testGroup.c_str())) {
							if (ImGui::Selectable(testGroup.c_str(), testGroup == testGroupName)) {
								setGUIValue_rendering<std::string>("testGroupName", testGroup);
							}
						}
					}
					ImGui::EndCombo();
				}
			}
		}
		BlockType blockType = getGUIValue_rendering<BlockType>("blockType");
		{
			std::lock_guard mux(blockTypesMux);
			auto iter = blockTypes.find(blockType);
			std::string blockTypeName = "NONE";
			if (iter != blockTypes.end()) blockTypeName = iter->second;
			{
				if (ImGui::BeginCombo("##blockTypeSelector", blockTypeName.c_str())) {
					static ImGuiTextFilter filter;
					if (ImGui::IsWindowAppearing()) {
						ImGui::SetKeyboardFocusHere();
						filter.Clear();
					}
					ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
					filter.Draw("##Filter", -FLT_MIN);

					for (const std::pair<circuit_id_t, std::string>& blockTypeIter : blockTypes)  {
						if (filter.PassFilter(blockTypeIter.second.c_str())) {
							if (ImGui::Selectable(blockTypeIter.second.c_str(), blockTypeIter.first == blockType)) {
								setGUIValue_rendering<BlockType>("blockType", (BlockType)blockTypeIter.first);
							}
						}
					}
					ImGui::EndCombo();
				}
			}
		}
		std::lock_guard mux(testGroupCopyMux);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, GUIColors::BACKGROUND);
		ifGui (ImGui::BeginChild("##mainView", ImVec2(ImGui::GetContentRegionAvail().x - 200, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_ResizeX, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse), ImGui::PopStyleColor()) {
			renderViewport(blockType, testGroupName);
		}
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_ChildBg, GUIColors::BACKGROUND);
		ifGui (ImGui::BeginChild("##SideBar"), ImGui::PopStyleColor()) {
			renderSideBar(blockType, testGroupName);
		}
		ImGui::EndChild();
	}
	if (!open) {
		getMainWindow().destroyWidget(this->getWidgetId());
	}
	ImGui::End();
}

void CircuitTestWidget::renderViewport(BlockType blockType, const std::string& testGroupName) {
	ImGui::SetScrollX(0);
	ImGui::SetScrollY(0);
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
	ImVec2 mousePos = ImGui::GetMousePos();
	mousePos = ImVec2(mousePos.x - viewportWindowScreenPos.x, mousePos.y - viewportWindowScreenPos.y);

	setGUIValue_rendering("AspectRatio", viewportPanelSize.x / viewportPanelSize.y);
	setGUIValue_rendering("MouseLeftDown", ImGui::IsMouseDown(ImGuiMouseButton_Left));
	setGUIValue_rendering("MouseRightDown", ImGui::IsMouseDown(ImGuiMouseButton_Right));
	setGUIValue_rendering("MousePosition", Vec2(mousePos.x / viewportPanelSize.x, mousePos.y / viewportPanelSize.y));
	setGUIValue_rendering("MouseInView", isHovered);
	setGUIValue_rendering("CircuitViewSize", Vec2(viewportPanelSize.x, viewportPanelSize.y));
	setGUIValue_rendering("CircuitViewIsFocused", ImGui::IsItemFocused());

	ImDrawList* drawlist = ImGui::GetWindowDrawList();
	const int offset = 2;
	const int padding = 4;

	FPosition viewportCenter = getGUIValue_rendering<FPosition>("ViewportCenter");
	FVector viewportCellSize = getGUIValue_rendering<FVector>("ViewportCellSize");
	ImVec2 pixPerCell = ImVec2(viewportPanelSize.x / viewportCellSize.dx, viewportPanelSize.y / viewportCellSize.dy);
	{
		const auto* renderText = MainRenderer::get().getTextOnViewport(circuitView->getViewportId());
		if (renderText) {
			for (const auto& pair : *renderText) {
				ImVec2 textPos = viewportWindowPos + viewportPanelSize / 2 - ImVec2(
					pixPerCell.x * (viewportCenter.x - pair.second.pos.x),
					pixPerCell.y * (viewportCenter.y - pair.second.pos.y)
				);
				ImGui::SetCursorPos(textPos);
				float curScale = ImGui::GetCurrentWindow()->FontWindowScale;
				if (pair.second.scale != 0) {
					if (pair.second.scale > 0) {
						ImGui::SetWindowFontScale(pair.second.scale * pixPerCell.x * 0.02);
					}
					if (pair.second.scale < 0) {
						ImGui::SetWindowFontScale(-pair.second.scale * curScale);
					}
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
					ImGui::Text("%s", pair.second.text.c_str());
					ImGui::PopStyleColor();
				}
				ImGui::SetWindowFontScale(curScale);
			}
		}
	}
	drawlist->ChannelsSplit(2);
		ImVec2 simControlsSize;
		{
			drawlist->ChannelsSetCurrent(1);
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
			drawlist->ChannelsSetCurrent(0);
			drawlist->AddRectFilled(
				{ viewportWindowScreenPos.x + offset, viewportWindowScreenPos.y + offset },
				{ viewportWindowScreenPos.x + simControlsSize.x + padding * 2 + offset, viewportWindowScreenPos.y + simControlsSize.y + padding * 2 + offset },
				ImColor(0.f, 0.f, 0.f, 0.2f),
				5.f
			);
		}
		drawlist->ChannelsMerge();
	std::string statusBarText = getGUIValue_rendering<std::string>("StatusBar");
	if (!statusBarText.empty()) {
		ImVec2 textSize = ImGui::CalcTextSize(statusBarText.data());
		drawlist->AddRectFilled(
			ImVec2(viewportPanelSize.x / 2.f, viewportPanelSize.y) + viewportWindowScreenPos - textSize / 2.f - ImVec2(padding, 50 +padding),
			ImVec2(viewportPanelSize.x / 2.f, viewportPanelSize.y) + viewportWindowScreenPos + textSize / 2.f - ImVec2(-padding, 50 -padding),
			ImColor(0.9f, 0.9f, 0.9f, 0.9f),
			5.f
		);
		ImGui::SetCursorScreenPos(ImVec2(viewportPanelSize.x / 2.f, viewportPanelSize.y) + viewportWindowScreenPos - textSize / 2.f - ImVec2(0, 50));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImGui::Text("%s", statusBarText.data());
		ImGui::PopStyleColor();
	}
}

void CircuitTestWidget::renderSideBar(BlockType blockType, const std::string& testGroupName) {
	// renderDataList
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, GUIColors::WIDGET_BACKGROUND);
	ifGui (ImGui::BeginChild("Render Data", ImGui::GetContentRegionAvail(), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_MenuBar),
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	) {
		if (ImGui::Button("Run All")) {
			if (testGroupRunner != std::nullopt && getGUIValue_rendering<BlockType>("blockType") != BlockType::NONE) {
						App::runOnMain([this](){ testGroupRunner->runAllTests(); });
			} else {
				getMainWindow().logError("Test group and circuit must be loaded before testing!");
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Edit")) {
			getMainWindow().log("This should edit the test.");
		}
		// if (ImGui::BeginMenuBar()) {
		// 	if (ImGui::BeginMenu("New")) {
		// 		if (ImGui::MenuItem("Texture")) {

		// 		}
		// 		if (ImGui::MenuItem("Shape")) { // if this is selected
		// 			getMainWindow().logError("Shape render data not real yet!");
		// 		}
		// 		ImGui::EndMenu();
		// 	}
		// 	ImGui::EndMenuBar();
		// }

		for (unsigned int index = 0; index < testGroupCopy.testCases.size(); index++) {
			CircuitTestGroup::TestCase* testCase = &testGroupCopy.testCases[index];
			ImGui::PushID(index);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, (index % 2 ==0) ? GUIColors::WIDGET_ALTERNATING_BACKGROUND_1 : GUIColors::WIDGET_ALTERNATING_BACKGROUND_2);
			ifGui (ImGui::BeginChild("Test Case", ImVec2(ImGui::GetContentRegionAvail().x, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding),
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			) {
				ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
				if(ImGui::Button("Run")) {
					if (testGroupRunner != std::nullopt && getGUIValue_rendering<BlockType>("blockType") != BlockType::NONE) {
						std::string testCaseName = testCase->name;
						App::runOnMain([this, testCaseName](){ testGroupRunner->runTest(testCaseName); });
					} else {
						getMainWindow().logError("Test group and circuit must be loaded before testing!");
					}
				}
				ImGui::SameLine();
				if (ImGui::TreeNodeEx(testCase->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::PopStyleVar();

					for (unsigned int commandIndex = 0; commandIndex < testCase->testCommands.size(); commandIndex++) {
						CircuitTestGroup::TestCommand* testCommand = &testCase->testCommands[commandIndex];
						if (testCommand->type != CircuitTestGroup::TICK_STEP) {
							if (ImGui::TreeNodeEx(CircuitTestGroup::getTestCommandTypeString(testCommand->type).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
								for (unsigned int stateIndex = 0; stateIndex < testCommand->states.size(); stateIndex++) {
									std::string stateStr = testCommand->states[stateIndex].first;
									stateStr.append(": ");
									stateStr.append(std::to_string((unsigned int)testCommand->states[stateIndex].second));
									ImGui::TreeNodeEx(stateStr.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
									ImGui::TableNextColumn();
								}
								ImGui::TreePop();
							}

						} else { // testCommand->type == CircuitTestGroup::TICK_STEP
							std::string commandStr = CircuitTestGroup::getTestCommandTypeString(testCommand->type);
							commandStr.append(": ");
							commandStr.append(std::to_string(testCommand->ticks));
							ImGui::TreeNodeEx(commandStr.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
						}
					}
					ImGui::TreePop();

				} else {
					ImGui::PopStyleVar();
				}
			}
			ImGui::EndChild();
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
}

void CircuitTestWidget::update() {
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
	if (getMainWindow().isPressingKeybind("Keybinds/Simulation/Transverse Simulation")) {
		if (dynamic_cast<const TreeTraversal*>(circuitView->getToolManager().getCircuitTool()) == nullptr) {
			lastToolStack = circuitView->getToolManager().getStack();
			circuitView->getToolManager().selectTool(std::make_shared<TreeTraversal>(getEnvironment()));
		}
	} else if (dynamic_cast<const TreeTraversal*>(circuitView->getToolManager().getCircuitTool()) != nullptr) {
		circuitView->getToolManager().selectStack(lastToolStack);
	}
}
