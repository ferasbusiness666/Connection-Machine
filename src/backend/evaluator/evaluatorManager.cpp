#include "evaluatorManager.h"

#include "backend/circuit/circuitManager.h"
#include "evaluator.h"

EvaluatorManager::EvaluatorManager(DataUpdateEventManager& dataUpdateEventManager) : dataUpdateEventManager(dataUpdateEventManager), evaluatorIdProvider(1) {}
EvaluatorManager::~EvaluatorManager() = default;

Evaluator* EvaluatorManager::getEvaluator(evaluator_id_t id) {
	auto iter = evaluators.find(id);
	if (iter == evaluators.end()) return nullptr;
	return &iter->second;
}
const Evaluator* EvaluatorManager::getEvaluator(evaluator_id_t id) const {
	auto iter = evaluators.find(id);
	if (iter == evaluators.end()) return nullptr;
	return &iter->second;
}

evaluator_id_t EvaluatorManager::createNewEvaluator(CircuitManager& circuitManager, circuit_id_t circuitId) {
	if (!circuitManager.getCircuit(circuitId)) {
		logError("Could not make evaluator because circuit {} was not found", "EvaluatorManager::createNewEvaluator", circuitId);
		return 0;
	}
	evaluator_id_t id = evaluatorIdProvider.getNewId();
	evaluators.try_emplace(
		id,
		id, circuitManager, circuitId, dataUpdateEventManager
	);
	dataUpdateEventManager.sendEvent("addressTreeMakeBranch");
	return id;
}

void EvaluatorManager::destroyEvaluator(evaluator_id_t id) {
	auto iter = evaluators.find(id);
	if (iter != evaluators.end()) {
		evaluatorIdProvider.releaseId(id);
		evaluators.erase(iter);
	}
}

void EvaluatorManager::applyDiff(DifferenceSharedPtr difference, circuit_id_t circuitId) {
	for (auto& [id, evaluator] : evaluators) {
		evaluator.makeEdit(difference, circuitId);
	}
}

nlohmann::json EvaluatorManager::dumpState() const /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json stateJson;
	for (const auto& [evaluatorId, evaluator] : evaluators) {
		stateJson.push_back(evaluator.dumpState());
	}
	return stateJson;
}
