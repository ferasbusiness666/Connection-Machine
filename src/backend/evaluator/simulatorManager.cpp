#include "simulatorManager.h"

#include "backend/circuit/circuitManager.h"
#include "simulator/evalLogicSimulator.h"

SimulatorManager::SimulatorManager(CircuitManager& circuitManager, DataUpdateEventManager& dataUpdateEventManager) :
	circuitManager(circuitManager), dataUpdateEventManager(dataUpdateEventManager), simulatorIdProvider(1) { }
SimulatorManager::~SimulatorManager() = default;

EvalLogicSimulator* SimulatorManager::getSimulator(simulator_id_t id) {
	auto iter = simulators.find(id);
	if (iter == simulators.end()) return nullptr;
	return iter->second.get();
}
const EvalLogicSimulator* SimulatorManager::getSimulator(simulator_id_t id) const {
	auto iter = simulators.find(id);
	if (iter == simulators.end()) return nullptr;
	return iter->second.get();
}

simulator_id_t SimulatorManager::createNewSimulator(circuit_id_t circuitId) {
	if (!circuitManager.getCircuit(circuitId)) {
		logError("Could not make simulator because circuit {} was not found", "SimulatorManager::createNewSimulator", circuitId);
		return 0;
	}
	simulator_id_t id = simulatorIdProvider.getNewId();
	simulators.try_emplace(id, std::make_unique<EvalLogicSimulator>(id, circuitManager, circuitId, dataUpdateEventManager));
	dataUpdateEventManager.sendEvent("addressTreeMakeBranch");
	return id;
}

void SimulatorManager::destroySimulator(simulator_id_t id) {
	auto iter = simulators.find(id);
	if (iter != simulators.end()) {
		simulatorIdProvider.releaseId(id);
		simulators.erase(iter);
	}
}

void SimulatorManager::applyDiff(DifferenceSharedPtr difference, circuit_id_t circuitId) {
	for (auto& [id, evaluator] : simulators) {
		// evaluator->makeEdit(difference, circuitId);
	}
}

nlohmann::json SimulatorManager::dumpState() const /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json stateJson;
	for (const auto& [simulatorId, evaluator] : simulators) {
		// stateJson.push_back(evaluator->dumpState());
	}
	return stateJson;
}
