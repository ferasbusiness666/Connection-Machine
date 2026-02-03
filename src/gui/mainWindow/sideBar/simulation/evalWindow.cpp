#include "evalWindow.h"

#include "backend/address.h"
#include "backend/circuit/circuitManager.h"
#include "backend/dataUpdateEventManager.h"
#include "backend/evaluator/simulatorManager.h"
#include "backend/evaluator/evaluator.h"

#include "gui/mainWindow/circuitView/circuitViewWidget.h"
#include "gui/mainWindow/mainWindow.h"

EvalWindow::EvalWindow(
	SimulatorManager& simulatorManager,
	CircuitManager& circuitManager,
	MainWindow& mainWindow,
	DataUpdateEventManager& dataUpdateEventManager,
	Rml::ElementDocument* document,
	Rml::Element* parent
) :
	menuTree(document, parent, true, false), dataUpdateEventReceiver(dataUpdateEventManager), simulatorManager(simulatorManager), circuitManager(circuitManager),
	mainWindow(mainWindow) {
	dataUpdateEventReceiver.linkFunction("evalUpdate", [this](const DataUpdateEventManager::EventData* event) { refreshSidebar(true); });
	dataUpdateEventReceiver.linkFunction("blockDataUpdate", [this](const DataUpdateEventManager::EventData* event) { refreshSidebar(true); });
	dataUpdateEventReceiver.linkFunction("circuitViewChangeSimulator", [this](const DataUpdateEventManager::EventData* event) { refreshSidebar(false); });
	// When the viewed circuit changes (due to navigating into/out of ICs), re-apply highlight.
	dataUpdateEventReceiver.linkFunction("circuitViewChangeCircuit", [this](const DataUpdateEventManager::EventData* event) { refreshSidebar(false); });
	dataUpdateEventReceiver.linkFunction("circuitCreatedSelect", std::bind(&EvalWindow::onCircuitCreatedSelect, this, std::placeholders::_1));
	menuTree.setListener(std::bind(&EvalWindow::updateSelected, this, std::placeholders::_1));
	refreshSidebar(true);
}

void EvalWindow::updateList() {
	std::vector<std::vector<std::string>> paths;
	for (const auto& pair : this->simulatorManager.getSimulators()) {
		std::vector<std::string> path({ pair.second->getSimulatorName() });
		makePaths(paths, path, pair.second->getCircuitId());
	}
	menuTree.setPaths(paths);
}

void EvalWindow::refreshSidebar(bool rebuildItems) {
	if (rebuildItems) updateList();
	if (!mainWindow.getActiveCircuitViewWidget()) return;
	CircuitView* view = mainWindow.getActiveCircuitViewWidget()->getCircuitView();
	if (!view) return;
	EvalLogicSimulator* activeEval = view->getSimulator();
	if (!activeEval) return;
	const simulator_id_t activeId = activeEval->getSimulatorId();
	const Address& address = view->getAddress();
	Rml::Element* root = menuTree.getRootElement();
	if (!root) return;
	Rml::ElementList rows;
	root->GetElementsByTagName(rows, "li");
	for (auto* r : rows) r->SetClass("selected", false);

	// Find the top-level simulator row matching the active simulator id
	Rml::Element* simulatorRow = nullptr;
	for (auto* r : rows) {
		std::string idAttr = r->GetId();
		if (idAttr.find('/') != std::string::npos) continue; // only top-level
		Rml::Element* labelDiv = nullptr;
		for (unsigned int i = 0; i < r->GetNumChildren(); ++i) {
			if (r->GetChild(i)->GetTagName() == "div") {
				labelDiv = r->GetChild(i);
				break;
			}
		}
		if (!labelDiv) continue;
		std::string label = labelDiv->GetInnerRML();
		std::stringstream ss(label);
		std::string word;
		unsigned int idParsed = 0;
		ss >> word >> idParsed;
		if (ss.fail() || word != "Eval") continue;
		if (simulator_id_t(idParsed) == activeId) {
			simulatorRow = r;
			break;
		}
	}
	if (!simulatorRow) return;

	// Descend according to the current address, matching by Position extracted from child labels "<Name>(x, y)"
	Rml::Element* selectedRow = simulatorRow; // default to simulator row if no deeper match
	Rml::Element* cur = simulatorRow;
	for (int i = 0; i < address.size(); ++i) {
		Position wanted = address.getPosition(i);
		// Find UL under current LI
		Rml::ElementList childLists;
		cur->GetElementsByTagName(childLists, "ul");
		if (childLists.empty()) break;
		Rml::Element* ul = childLists[0];
		Rml::Element* matchLi = nullptr;
		for (unsigned int c = 0; c < ul->GetNumChildren(); ++c) {
			Rml::Element* li = ul->GetChild(c);
			if (li->GetTagName() != "li") continue;
			Rml::Element* div = nullptr;
			for (unsigned int k = 0; k < li->GetNumChildren(); ++k) {
				if (li->GetChild(k)->GetTagName() == "div") {
					div = li->GetChild(k);
					break;
				}
			}
			if (!div) continue;
			std::string text = div->GetInnerRML();
			auto lparen = text.find_last_of('(');
			if (lparen == std::string::npos || text.empty() || text.back() != ')') continue;
			std::string coord = text.substr(lparen + 1, text.size() - lparen - 2);
			std::stringstream ps(coord);
			Position p{};
			char comma = 0;
			if (!(ps >> p.x)) continue;
			ps >> comma;
			if (comma != ',') continue;
			if (!(ps >> p.y)) continue;
			if (p == wanted) {
				matchLi = li;
				break;
			}
		}
		if (!matchLi) break; // stop descending if not found
		selectedRow = matchLi;
		cur = matchLi;
	}

	// Apply selection to the deepest matched row and expand ancestors
	selectedRow->SetClass("selected", true);
	Rml::Element* p = selectedRow->GetParentNode();
	while (p) {
		if (p->GetTagName() == "li") p->SetClass("collapsed", false);
		p = p->GetParentNode();
	}
}

void EvalWindow::makePaths(std::vector<std::vector<std::string>>& paths, std::vector<std::string>& path, circuit_id_t circuitId) {
	paths.push_back(path);
	const Circuit* circuit = circuitManager.getCircuit(circuitId).get();
	if (!circuit) {
		logError("Failed to get circuit from circuit id {}.", "EvalWindow::makePaths", circuit->getCircuitId());
		return;
	}
	for (std::pair<Position, circuit_id_t> pair : circuit->getEvaluator().getSubcircuits()) {
		path.push_back(circuitManager.getCircuit(pair.second)->getCircuitName() + pair.first.toString());
		makePaths(paths, path, pair.second);
		path.pop_back();
	}
}

void EvalWindow::updateSelected(std::string string) {
	std::vector<std::string> parts = stringSplit(string, '/');
	std::stringstream evalName(parts.front());
	std::string str;
	unsigned int simulatorId;
	evalName >> str >> simulatorId;
	Address address;
	for (unsigned int i = 1; i < parts.size(); i++) {
		const std::string& part = parts[i];
		size_t index = part.size();
		while (index != 0 && part[index - 1] != '(') index--;
		if (index == 0) continue;
		std::stringstream posString(part.substr(index, part.size() - index - 1));
		Position position{};
		char sep = 0;
		if (!(posString >> position.x)) continue;
		if (!(posString >> sep)) continue;
		if (!(posString >> position.y)) continue;
		address.appendPosition(position);
	}

	CircuitView* circuitView = mainWindow.getActiveCircuitViewWidget()->getCircuitView();
	circuitView->setSimulatoruator(simulator_id_t(simulatorId), address);
	refreshSidebar(false);
}

void EvalWindow::selectSimulatoruatorForCircuit(circuit_id_t circuitId) {
	for (auto& pair : simulatorManager.getSimulators()) {
		if (pair.second->getCircuitId() == circuitId) {
			simulator_id_t wantedId = pair.first;
			Rml::Element* root = menuTree.getRootElement();
			if (!root) return;
			Rml::ElementList rows;
			root->GetElementsByTagName(rows, "li");
			for (auto* r : rows) {
				std::string idAttr = r->GetId();
				if (idAttr.find('/') != std::string::npos) continue; // only top-level simulators
				Rml::Element* labelDiv = nullptr;
				for (unsigned int i = 0; i < r->GetNumChildren(); ++i) {
					if (r->GetChild(i)->GetTagName() == "div") {
						labelDiv = r->GetChild(i);
						break;
					}
				}
				if (!labelDiv) continue;
				std::string label = labelDiv->GetInnerRML();
				std::stringstream ss(label);
				std::string word;
				unsigned int idParsed = 0;
				ss >> word >> idParsed;
				if (ss.fail() || word != "Eval") continue;
				if (simulator_id_t(idParsed) != wantedId) continue;
				Rml::ElementList all;
				root->GetElementsByTagName(all, "li");
				for (auto* a : all) a->SetClass("selected", false);
				r->SetClass("selected", true);
				return;
			}
			return;
		}
	}
}

void EvalWindow::onCircuitCreatedSelect(const DataUpdateEventManager::EventData* event) { refreshSidebar(true); }
