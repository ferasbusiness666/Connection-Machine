#ifndef evalWindow_h
#define evalWindow_h

#include "backend/circuit/circuit.h"
#include "gui/helper/menuTree.h"

class SimulatorManager;
class CircuitManager;
class MainWindow;

class EvalWindow {
public:
	EvalWindow(
		SimulatorManager& simulatorManager,
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
	void selectSimulatoruatorForCircuit(circuit_id_t circuitId);
	void onCircuitCreatedSelect(const DataUpdateEventManager::EventData* event);

	MenuTree menuTree;
	DataUpdateEventManager::DataUpdateEventReceiver dataUpdateEventReceiver;
	MainWindow& mainWindow;
	SimulatorManager& simulatorManager;
	CircuitManager& circuitManager;
};

#endif /* evalWindow_h */