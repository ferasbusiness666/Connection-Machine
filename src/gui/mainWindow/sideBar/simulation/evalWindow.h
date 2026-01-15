#ifndef evalWindow_h
#define evalWindow_h

#include "backend/circuit/circuit.h"
#include "gui/helper/menuTree.h"

class EvaluatorManager;
class CircuitManager;
class MainWindow;

class EvalWindow {
public:
	EvalWindow(
		EvaluatorManager& evaluatorManager,
		CircuitManager& circuitManager,
		MainWindow& mainWindow,
		DataUpdateEventManager& dataUpdateEventManager,
		Rml::ElementDocument* document,
		Rml::Element* parent
	);

	void updateList();
	void refreshSidebar(bool rebuildItems = false);

private:
	void updateSelected(std::string string);
	void makePaths(std::vector<std::vector<std::string>>& paths, std::vector<std::string>& path/*, const EvalAddressTree& addressTree*/);
	void selectEvaluatorForCircuit(circuit_id_t circuitId);
	void onCircuitCreatedSelect(const DataUpdateEventManager::EventData& event);

	MenuTree menuTree;
	DataUpdateEventManager::DataUpdateEventReceiver dataUpdateEventReceiver;
	MainWindow& mainWindow;
	EvaluatorManager& evaluatorManager;
	CircuitManager& circuitManager;
};

#endif /* evalWindow_h */