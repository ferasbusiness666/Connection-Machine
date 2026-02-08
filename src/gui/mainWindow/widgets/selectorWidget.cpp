#include "selectorWidget.h"

#include "../mainWindow.h"

#include "imgui/imgui_stdlib.h"

SelectorWidget::SelectorWidget(WidgetId widgetId, MainWindow& mainWindow) :
	Widget(widgetId, mainWindow), dataUpdateEventReceiver(getBackend().getDataUpdateEventManager()) {
	{
		// =================================== Init all tree data ===================================
		std::lock_guard mux(pathsMux);
		// Blocks
		const BlockDataManager& blockDataManager = getBackend().getBlockDataManager();
		for (unsigned int type = 0; type < blockDataManager.maxBlockId(); type++) {
			const BlockData* blockData = blockDataManager.getBlockData((BlockType)(type + 1));
			if (blockData) {
				addPath("Blocks/" + blockData->getPath() + "/" + blockData->getName(), (BlockType)(type + 1));
			}
		}
		// Procedural Circuits
		for (const auto& iter : getBackend().getCircuitManager().getProceduralCircuitManager().getProceduralCircuits()) {
			addPath("Blocks/" + iter.second->getPath(), iter.second->getUUID());
		}
		addPath("Blocks/Other/Bus", "Other/Bus");
		// Tools
		for (const auto& iter : getMainWindow().getToolManagerManager().getAllTools()) {
			addPath("Tools/" + iter.first, iter.first);
		}
	}

	dataUpdateEventReceiver.linkFunction("blockDataUpdate", [this](const DataUpdateEventManager::EventData* event) {
		const DataUpdateEventManager::EventDataWithValue<BlockType>* data = event->cast<BlockType>();
		assert(data);
		const BlockData* blockData = getBackend().getBlockDataManager().getBlockData(data->get());
		std::lock_guard mux(pathsMux);
		addPath("Blocks/" + blockData->getPath() + "/" +blockData->getName(), data->get());
	});
	dataUpdateEventReceiver.linkFunction("proceduralCircuitPathUpdate", [this](const DataUpdateEventManager::EventData* event) {
		const DataUpdateEventManager::EventDataWithValue<std::string>* data = event->cast<std::string>();
		assert(data);
		SharedProceduralCircuit proceduralCircuit = getBackend().getCircuitManager().getProceduralCircuitManager().getProceduralCircuit(data->get());
		addPath("Blocks/" + proceduralCircuit->getPath(), proceduralCircuit->getPath());
	});
	dataUpdateEventReceiver.linkFunction("setToolModeUpdate", [](const DataUpdateEventManager::EventData* event) {

	});
	dataUpdateEventReceiver.linkFunction("setToolBlockTypeUpdate", [this](const DataUpdateEventManager::EventData* event) {
		const DataUpdateEventManager::EventDataWithValue<BlockType>* data = event->cast<BlockType>();
		assert(data);
		setGUIValue<BlockType>("selectedBlockType", data->get());
	});
	// dataUpdateEventReceiver.linkFunction("setToolBlockOrientationUpdate", [](const DataUpdateEventManager::EventData* event) {

	// });
	dataUpdateEventReceiver.linkFunction("setToolUpdate", [this](const DataUpdateEventManager::EventData* event) {
		const DataUpdateEventManager::EventDataWithValue<std::string>* data = event->cast<std::string>();
		assert(data);
		setGUIValue<std::string>("selectedTool", data->get());
	});
	setupGUIValue<BlockType>("selectedBlockType", BlockType::NONE, [this](const BlockType& blockType) { getMainWindow().getToolManagerManager().setBlock(blockType); });
	setupGUIValue<std::string>("selectedTool", "", [this](const std::string& toolPath) { getMainWindow().getToolManagerManager().setTool(toolPath); });
	setupGUIValue<std::string>("selectedProceduralCircuitOrBus", "", [this](const std::string& path) {
		std::lock_guard lock(proceduralCircuitOrBusParameterMux);
		auto pair = proceduralCircuitOrBusParameter.try_emplace(path);
		if (!pair.second) return;
		if (path == "Other/Bus") return;
		const std::string* uuid = getBackend().getCircuitManager().getProceduralCircuitManager().getProceduralCircuitUUID(path);
		if (uuid == nullptr) return;
		SharedProceduralCircuit proceduralCircuit = getBackend().getCircuitManager().getProceduralCircuitManager().getProceduralCircuit(*uuid);
		if (proceduralCircuit == nullptr) return;
		pair.first->second = proceduralCircuit->getParameterDefaults().parameters;
	});
	setupGUIValue<bool>("doCreateBlock", false, [this](const bool& doCreateBlock) {
		if (doCreateBlock) {
			const std::string& selectedProceduralCircuitOrBus = getGUIValue<std::string>("selectedProceduralCircuitOrBus");
			if (selectedProceduralCircuitOrBus == "") return;
			if (selectedProceduralCircuitOrBus == "Other/Bus") {
				std::lock_guard lock(proceduralCircuitOrBusParameterMux);
				auto iter = proceduralCircuitOrBusParameter.find("Other/Bus");
				assert(iter != proceduralCircuitOrBusParameter.end());
				getMainWindow().getToolManagerManager().setBlock(getBackend().getBlockDataManager().getBusBlock(
					std::get<unsigned int>(iter->second.at("Number of Ports")),
					std::get<unsigned int>(iter->second.at("Port Bit Widths"))
				));
			} else {
				std::lock_guard lock(proceduralCircuitOrBusParameterMux);
				auto iter = proceduralCircuitOrBusParameter.find(selectedProceduralCircuitOrBus);
				assert(iter != proceduralCircuitOrBusParameter.end());
				const std::string* uuid = getBackend().getCircuitManager().getProceduralCircuitManager().getProceduralCircuitUUID(selectedProceduralCircuitOrBus);
				assert(uuid);
				SharedProceduralCircuit proceduralCircuit = getBackend().getCircuitManager().getProceduralCircuitManager().getProceduralCircuit(*uuid);
				assert(proceduralCircuit);
				ProceduralCircuitParameters parameters;
				parameters.parameters = iter->second;
				circuit_id_t circuitId = proceduralCircuit->getCircuitId(parameters);
				const CircuitBlockData* circuitBlockData = getBackend().getCircuitManager().getCircuitBlockDataManager().getCircuitBlockData(circuitId);
				assert(circuitBlockData);
				getMainWindow().getToolManagerManager().setBlock(circuitBlockData->getBlockType());
			}
		}
	});
	// setupGUIValue<std::string>("selectedToolMode", BlockType::NONE, [this](const BlockType& blockType) {
	// 	getMainWindow().getToolManagerManager().setBlock(blockType);
	// });
}

void SelectorWidget::addPath(const std::string& path, const std::variant<BlockType, std::string>& data) {
	if (std::holds_alternative<BlockType>(data)) logInfo("adding {}: {}", "", path, std::get<BlockType>(data));
	else logInfo("adding {}: {}", "", path, std::get<std::string>(data));
	auto pair = paths.emplace(data, path);
	if (!pair.second) {
		if (pair.first->second == path) return;
		root.removePath(pair.first->second);
		pair.first->second = path;
	}
	root.addPath(path, data);
}

void SelectorWidget::createTree(const SelectorTreeNode& node, const std::string& rootString) {
	for (auto& pair : node.children) {
		if (pair.second.data.has_value()) {
			// leaf (never children)
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
			if (std::holds_alternative<BlockType>(pair.second.data.value())) {
				assert(rootString == "Blocks");
				if (getGUIValue_rendering<BlockType>("selectedBlockType") == std::get<BlockType>(pair.second.data.value())) {
					flags |= ImGuiTreeNodeFlags_Selected;
				}
			} else if (rootString == "Blocks") {
				if (getGUIValue_rendering<std::string>("selectedProceduralCircuitOrBus") == std::get<std::string>(pair.second.data.value())) {
					flags |= ImGuiTreeNodeFlags_Selected;
				};
			} else if (rootString == "Tools") {
				assert(std::holds_alternative<std::string>(pair.second.data.value()));
				if (getGUIValue_rendering<std::string>("selectedTool") == std::get<std::string>(pair.second.data.value())) {
					flags |= ImGuiTreeNodeFlags_Selected;
				}
			}

			if (ImGui::TreeNodeEx(pair.first.c_str(), flags)) {
				if (ImGui::IsItemClicked()) {
					if (std::holds_alternative<BlockType>(pair.second.data.value())) {
						assert(rootString == "Blocks");
						setGUIValue_rendering<BlockType>("selectedBlockType", std::get<BlockType>(pair.second.data.value()));
						setGUIValue_rendering<std::string>("selectedProceduralCircuitOrBus", "");
					} else if (rootString == "Blocks") {
						const std::string& data = std::get<std::string>(pair.second.data.value());
						setGUIValue_rendering<BlockType>("selectedBlockType", BlockType::NONE);
						setGUIValue_rendering<std::string>("selectedProceduralCircuitOrBus", data);
					} else if (rootString == "Tools") {
						assert(std::holds_alternative<std::string>(pair.second.data.value()));
						setGUIValue_rendering("selectedTool", std::get<std::string>(pair.second.data.value()));
					}
				}
				ImGui::TreePop();
			}
		} else {
			// non-leaf (sometimes children)
			if (ImGui::TreeNodeEx(pair.first.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				if (rootString.empty()) {
					createTree(pair.second, pair.first);
				} else {
					createTree(pair.second, rootString);
				}
				ImGui::TreePop();
			}
		}
	}
}

unsigned int slowStep = 1;
unsigned int fastStep = 10;

SelectorWidget::~SelectorWidget() { }
void SelectorWidget::render() {
	ImGui::SetNextWindowDockID(getMainWindow().getDockLeftId(), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(100, 300), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(getWidgetIdStr().c_str())) {
		const std::string& selectedProceduralCircuitOrBus = getGUIValue_rendering<std::string>("selectedProceduralCircuitOrBus");
		if (selectedProceduralCircuitOrBus != "") {
			if (selectedProceduralCircuitOrBus == "Other/Bus") {
				if (ImGui::BeginChild("Bus Creator", { 0, 0 }, ImGuiChildFlags_AutoResizeY)) {
					std::lock_guard lock(proceduralCircuitOrBusParameterMux);
					auto pair = proceduralCircuitOrBusParameter.try_emplace("Other/Bus");
					auto numberOfPorts = pair.first->second.emplace("Number of Ports", (unsigned int)8);
					assert(std::holds_alternative<unsigned int>(numberOfPorts.first->second));
					ImGui::InputScalar("Number of Ports", ImGuiDataType_U32, &std::get<unsigned int>(numberOfPorts.first->second), &slowStep, &fastStep);
					auto portBitWidths = pair.first->second.emplace("Port Bit Widths", (unsigned int)1);
					ImGui::InputScalar("Port Bit Widths", ImGuiDataType_U32, &std::get<unsigned int>(portBitWidths.first->second), &slowStep, &fastStep);
					bool a = ImGui::Button("Create Bus");
					setGUIValue_rendering<bool>("doCreateBlock", a);
				}
				ImGui::EndChild();
			} else {
				if (ImGui::BeginChild("Procedural Circuit", { 0, 0 }, ImGuiChildFlags_AutoResizeY)) {
					ImGui::Text("(%s)", selectedProceduralCircuitOrBus.c_str());
					std::lock_guard lock(proceduralCircuitOrBusParameterMux);
					auto iter = proceduralCircuitOrBusParameter.find(selectedProceduralCircuitOrBus);
					if (iter == proceduralCircuitOrBusParameter.end()) {
						logError("Could not find {} in proceduralCircuitOrBusParameter. This should have been setup before this code runs.", "SelectorWidget::render");
					} else {
						for (auto& parameter : iter->second) {
							if (std::holds_alternative<int>(parameter.second)) {
								ImGui::InputScalar(parameter.first.c_str(), ImGuiDataType_S32, &std::get<int>(parameter.second), &slowStep, &fastStep);
							} else if (std::holds_alternative<unsigned int>(parameter.second)) {
								ImGui::InputScalar(parameter.first.c_str(), ImGuiDataType_U32, (int*)&std::get<unsigned int>(parameter.second), &slowStep, &fastStep);
							} else if (std::holds_alternative<float>(parameter.second)) {
								ImGui::InputScalar(parameter.first.c_str(), ImGuiDataType_Float, &std::get<float>(parameter.second), &slowStep, &fastStep);
							} else if (std::holds_alternative<std::string>(parameter.second)) {
								ImGui::InputText(parameter.first.c_str(), &std::get<std::string>(parameter.second));
							}
						}
					}
					setGUIValue_rendering<bool>("doCreateBlock", ImGui::Button("Create Circuit"));
				}
				ImGui::EndChild();
			}
		}
		std::lock_guard mux(pathsMux);
		createTree(root);
	}
	ImGui::End();
}

/*
SelectorWindow::SelectorWindow(
	BlockDataManager& blockDataManager,
	DataUpdateEventManager& dataUpdateEventManager,
	ProceduralCircuitManager& proceduralCircuitManager,
	ToolManagerManager& toolManagerManager,
	Rml::ElementDocument* document
) :
	blockDataManager(blockDataManager), proceduralCircuitManager(proceduralCircuitManager), toolManagerManager(toolManagerManager),
	dataUpdateEventReceiver(dataUpdateEventManager), document(document) {
	Rml::Element* itemTreeParent = document->GetElementById("item-selection-tree");
	menuTree.emplace(document, itemTreeParent, false);
	menuTree->setListener(std::bind(&SelectorWindow::updateSelected, this, std::placeholders::_1));
	// In Blocks/Tools tree, prevent selecting category dropdown parents ("Blocks" / "Tools")
	menuTree->disallowParentSelection(true);
	dataUpdateEventReceiver.linkFunction("blockDataUpdate", [this](const DataUpdateEventManager::EventData* event) { refreshSidebar(true); });
	dataUpdateEventReceiver.linkFunction("proceduralCircuitPathUpdate", [this](const DataUpdateEventManager::EventData* event) { refreshSidebar(true); });

	Rml::Element* modeTreeParent = document->GetElementById("mode-selection-tree");
	modeList.emplace(document, modeTreeParent, [this](const std::string& item, Rml::Element* element) {
		element->AppendChild(std::move(this->document->CreateTextNode(item)));
	});
	modeList->addEventListener(Rml::EventId::Click, std::bind(&SelectorWindow::updateSelectedMode, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("setToolModeUpdate", [this](const DataUpdateEventManager::EventData* event) { updateToolModeOptions(); });

	dataUpdateEventReceiver.linkFunction("setToolUpdate", [this](const DataUpdateEventManager::EventData* event) { refreshSidebar(false); });

	parameterMenu = document->GetElementById("parameter-menu");
	parameterMenu->GetElementById("reset-parameters")->AddEventListener(Rml::EventId::Click, new EventPasser([this](Rml::Event& event) { setupParameterMenu(); }));
	parameterMenu->GetElementById("create-block")->AddEventListener(Rml::EventId::Click, new EventPasser([this](Rml::Event& event) {
		if (selectedProceduralCircuit) {
			Rml::Element* parametersElement = parameterMenu->GetElementById("parameter-menu-parameters");

			ProceduralCircuitParameters proceduralCircuitParameters;
			for (unsigned int i = 0; i < parametersElement->GetNumChildren(); ++i) {
				Rml::ElementList elements;
				parametersElement->GetChild(i)->GetElementsByClassName(elements, "parameter-name");
				std::string key = elements[0]->GetInnerRML();
				key.pop_back();
				elements.clear();
				parametersElement->GetChild(i)->GetElementsByClassName(elements, "parameter-input");
				Rml::ElementFormControlInput* parameterInput = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(elements[0]);
				std::string str = parameterInput->GetValue();
				try {
					int value = std::stoi(str);
					proceduralCircuitParameters.parameters.emplace(std::move(key), value);
				} catch (std::exception const& ex) {
					logError("Invalid parameter for {}: {}. {}", "", key, str, ex.what());
					return;
				}
			}
			this->toolManagerManager.setBlock(selectedProceduralCircuit->getBlockType(proceduralCircuitParameters));
		} else if (selectedBus) {
			Rml::Element* parametersElement = parameterMenu->GetElementById("parameter-menu-parameters");
			assert(parametersElement->GetNumChildren() == 2);
			Rml::ElementList elements;
			parametersElement->GetChild(0)->GetElementsByClassName(elements, "parameter-input");
			Rml::ElementFormControlInput* numInputsInput = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(elements[0]);
			elements.clear();
			parametersElement->GetChild(1)->GetElementsByClassName(elements, "parameter-input");
			Rml::ElementFormControlInput* inputBitWidthInput = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(elements[0]);
			std::string numInputsStr = numInputsInput->GetValue();
			std::string inputBitWidthStr = inputBitWidthInput->GetValue();
			try {
				int numInputs = std::stoi(numInputsStr);
				int inputBitWidth = std::stoi(inputBitWidthStr);
				this->toolManagerManager.setBlock(this->blockDataManager.getBusBlock(numInputs, inputBitWidth));
			} catch (std::exception const& ex) {
				logError("Invalid bus parameters: {} inputs of {} bits each. {}", "", numInputsStr, inputBitWidthStr, ex.what());
				return;
			}
		}
	}));

	refreshSidebar(true);
}

void SelectorWindow::updateList() {
	std::vector<std::vector<std::string>> paths;
	for (unsigned int blockType = 1; blockType <= blockDataManager.maxBlockId(); blockType++) {
		if (!blockDataManager.isPlaceable((BlockType)blockType)) continue;
		std::vector<std::string>& path = paths.emplace_back(1, "Blocks");
		stringSplitInto(blockDataManager.getPath((BlockType)blockType), '/', path);
		path.push_back(blockDataManager.getName((BlockType)blockType));
	}
	for (const auto& iter : toolManagerManager.getAllTools()) {
		std::vector<std::string>& path = paths.emplace_back(1, "Tools");
		stringSplitInto(iter.first, '/', path);
	}
	for (const auto& iter : proceduralCircuitManager.getProceduralCircuits()) {
		std::vector<std::string>& path = paths.emplace_back(1, "Blocks");
		stringSplitInto(iter.second->getPath(), '/', path);
	}
	{
		std::vector<std::string>& path = paths.emplace_back(1, "Blocks");
		stringSplitInto("Other/Bus", '/', path);
	}
	menuTree->setPaths(paths);
}

void SelectorWindow::updateToolModeOptions() {
	auto modes = toolManagerManager.getActiveToolModes();
	modeList->setItems(modes.value_or(std::vector<std::string>()));
	highlightActiveMode();
}

void SelectorWindow::refreshSidebar(bool rebuildItems) {
	if (rebuildItems) updateList();
	highlightActiveToolInSidebar();
	updateToolModeOptions();
}

void SelectorWindow::highlightActiveToolInSidebar() {
	if (!menuTree) return;
	const std::string& activeTool = toolManagerManager.getActiveTool();
	if (activeTool.empty()) return;
	std::string activeToolId = std::string("Tools/") + activeTool + "-menu";
	if (Rml::Element* activeElement = document->GetElementById(activeToolId)) {
		if (Rml::Element* itemRoot = document->GetElementById("item-selection-tree")) {
			Rml::ElementList rows;
			itemRoot->GetElementsByTagName(rows, "li");
			for (auto* row : rows) row->SetClass("selected", false);
		}
		activeElement->SetClass("selected", true);
		// Expand ancestors
		Rml::Element* p = activeElement->GetParentNode();
		while (p) {
			if (p->GetTagName() == "li") p->SetClass("collapsed", false);
			p = p->GetParentNode();
		}
	}
	if (selectedBus) {
		std::string elementId = std::string("Blocks/") + "Other/Bus" + "-menu";
		Rml::Element* blockElement = document->GetElementById(elementId);
		if (blockElement) {
			blockElement->SetClass("selected", true);
			Rml::Element* p = blockElement->GetParentNode();
			while (p) {
				if (p->GetTagName() == "li") p->SetClass("collapsed", false);
				p = p->GetParentNode();
			}
		}
	} else if (selectedProceduralCircuit) {
		std::string elementId = std::string("Blocks/") + selectedProceduralCircuit->getPath() + "-menu";
		Rml::Element* blockElement = document->GetElementById(elementId);
		if (blockElement) {
			blockElement->SetClass("selected", true);
			Rml::Element* p = blockElement->GetParentNode();
			while (p) {
				if (p->GetTagName() == "li") p->SetClass("collapsed", false);
				p = p->GetParentNode();
			}
		}
	} else {
		BlockType selectedBlock = toolManagerManager.getSelectedBlock();
		if (selectedBlock != BlockType::NONE) {
			std::string blockPath = blockDataManager.getPath(selectedBlock);
			std::string blockName = blockDataManager.getName(selectedBlock);
			std::string elementId = "Blocks/";
			if (!blockPath.empty()) elementId += blockPath + "/";
			elementId += blockName + "-menu";
			Rml::Element* blockElement = document->GetElementById(elementId);
			if (blockElement) {
				blockElement->SetClass("selected", true);
				Rml::Element* p = blockElement->GetParentNode();
				while (p) {
					if (p->GetTagName() == "li") p->SetClass("collapsed", false);
					p = p->GetParentNode();
				}
			}
		}
	}
}

void SelectorWindow::highlightActiveMode() {
	for (unsigned int i = 0; i < modeList->getSize(); i++) {
		modeList->getItem(i)->SetClass("selected", false);
	}
	std::optional<std::string> mode = toolManagerManager.getActiveToolMode();
	if (!mode) return;
	Rml::Element* item = modeList->getItem(*mode);
	if (item) item->SetClass("selected", true);
}

void SelectorWindow::updateSelected(const std::string& string) {
	std::vector parts = stringSplit(string, '/');
	if (parts.size() <= 1) return;
	if (parts[0] == "Blocks") {
		selectedProceduralCircuit = nullptr; // either it will be set or this should go away!
		selectedBus = false;
		std::string path = string.substr(7, string.size() - 7);
		BlockType blockType = blockDataManager.getBlockType(path);
		if (blockType == BlockType::NONE) {
			const std::string* uuid = proceduralCircuitManager.getProceduralCircuitUUID(path);
			if (uuid) {
				selectedProceduralCircuit = proceduralCircuitManager.getProceduralCircuit(*uuid);
				if (selectedProceduralCircuit) setupProceduralCircuitParameterMenu();
				else logError("unknown block with path: {}", "SelectorWindow", path);
			} else if (path == "Other/Bus") {
				selectedBus = true;
				setupBusParameterMenu();
			}
		}
		toolManagerManager.setBlock(blockType);
		if (!(selectedProceduralCircuit || selectedBus)) hideParameterMenu();
	} else if (parts[0] == "Tools") {
		std::string toolPath = string.substr(6, string.size() - 6);
		toolManagerManager.setTool(toolPath);
	} else {
		logError("Do not recognize cadegory {}", "SelectorWindow", parts[0]);
	}
	refreshSidebar(false);
}

void SelectorWindow::updateSelectedMode(const std::string& string) { toolManagerManager.setMode(string); }

void SelectorWindow::setupParameterMenu() {
	if (selectedProceduralCircuit) {
		setupProceduralCircuitParameterMenu();
	} else if (selectedBus) {
		setupBusParameterMenu();
	} else {
		hideParameterMenu();
	}
}

void SelectorWindow::setupProceduralCircuitParameterMenu() {
	if (!selectedProceduralCircuit) {
		hideParameterMenu();
		return;
	}

	addParametersToParameterMenu(selectedProceduralCircuit->getParameterDefaults(), selectedProceduralCircuit->getProceduralCircuitName());
}

void SelectorWindow::setupBusParameterMenu() {
	if (!selectedBus) {
		hideParameterMenu();
		return;
	}
	ProceduralCircuitParameters parameters;
	parameters.parameters.emplace("Number of Ports", 8);
	parameters.parameters.emplace("Port Bit Widths", 1);
	addParametersToParameterMenu(parameters, "Bus");
}

void SelectorWindow::addParametersToParameterMenu(const ProceduralCircuitParameters& parameters, const std::string& title) {
	parameterMenu->SetClass("invisible", false);
	parameterMenu->GetElementById("parameter-menu-active")->SetInnerRML(title);
	Rml::Element* parametersElement = parameterMenu->GetElementById("parameter-menu-parameters");

	while (parametersElement->GetNumChildren() > 0) parametersElement->RemoveChild(parametersElement->GetChild(0));

	for (const std::pair<std::string, int>& pair : parameters.parameters) {
		Rml::ElementPtr parameterDiv = makeParameterElement(pair.first, pair.second);
		parametersElement->AppendChild(std::move(parameterDiv));
	}
}

Rml::ElementPtr SelectorWindow::makeParameterElement(const std::string& name, int defaultValue) {
	Rml::ElementPtr parameterDiv = document->CreateElement("div");
	parameterDiv->SetClass("parameter", true);

	Rml::ElementPtr parameterNameElement = document->CreateElement("span");
	parameterNameElement->SetInnerRML(name + ":");
	parameterNameElement->SetClass("parameter-name", true);

	Rml::XMLAttributes parameterInputAttributes;
	parameterInputAttributes["type"] = "text";
	parameterInputAttributes["maxlength"] = "8";
	parameterInputAttributes["size"] = "8";
	Rml::ElementPtr parameterInputElement = Rml::Factory::InstanceElement(document, "input", "input", parameterInputAttributes);
	parameterInputElement->SetClass("parameter-input", true);
	Rml::ElementFormControlInput* parameterInput = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(parameterInputElement.get());
	parameterInput->SetValue(std::to_string(defaultValue));

	parameterDiv->AppendChild(std::move(parameterNameElement));
	parameterDiv->AppendChild(std::move(parameterInputElement));
	return parameterDiv;
}

void SelectorWindow::hideParameterMenu() { parameterMenu->SetClass("invisible", true); }
*/