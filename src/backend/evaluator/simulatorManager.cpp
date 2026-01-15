#include "simulatorManager.h"

#include "backend/circuit/circuitManager.h"
#include "evaluator.h"

evaluator_id_t SimulatorManager::createNewSimulator(CircuitManager& circuitManager, circuit_id_t circuitId) {
	if (!circuitManager.getCircuit(circuitId)) {
		logError("Could not make simulator because circuit {} was not found", "SimulatorManager::createNewSimulator", circuitId);
		return 0;
	}
	evaluator_id_t id = evaluatorIdProvider.getNewId();
	simulators.try_emplace(
		id
		// id, circuitManager, circuitId, dataUpdateEventManager
	);
	dataUpdateEventManager.sendEvent("addressTreeMakeBranch");
	return id;
}

void SimulatorManager::applyDiff(DifferenceSharedPtr difference, circuit_id_t circuitId) {
	for (auto& [id, evaluator] : simulators) {
		// evaluator->makeEdit(difference, circuitId);
	}
}

nlohmann::json SimulatorManager::dumpState() const /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json stateJson;
	for (const auto& [evaluatorId, evaluator] : simulators) {
		// stateJson.push_back(evaluator->dumpState());
	}
	return stateJson;
}
