#include "evaluator.h"

#include "backend/circuit/circuitManager.h"
#include "evaluatorInternal.h"

Evaluator::~Evaluator() = default;

Evaluator::Evaluator(
	evaluator_id_t evaluatorId,
	CircuitManager& circuitManager,
	circuit_id_t circuitId,
	DataUpdateEventManager& dataUpdateEventManager
) :
	evaluatorId(evaluatorId),
	circuitManager(circuitManager),
	dataUpdateEventManager(dataUpdateEventManager),
	receiver(dataUpdateEventManager),
	circuitId(circuitId),
	evaluatorInternal(std::make_unique<EvaluatorInternal>(circuitId, circuitManager, receiver)),
	evalLogicSimulator(evaluatorId.get(), circuitManager.getBlockDataManager(), *evaluatorInternal, dataUpdateEventManager)
{
	const Circuit* circuit = circuitManager.getCircuit(circuitId).get();
	assert(circuit);

	logInfo("Creating Evaluator with ID {} for Circuit ID {}", "Evaluator", evaluatorId, circuitId);
	const BlockContainer& blockContainer = circuit->getBlockContainer();
	const Difference difference = blockContainer.getCreationDifference();

	makeEdit(std::make_shared<Difference>(difference), circuitId);
}

std::string Evaluator::getEvaluatorName() const {
	std::optional<circuit_id_t> circuitId = this->circuitId;
	if (!circuitId.has_value()) {
		logError("EvalCircuit with id {} has no circuitId", "Evaluator::getEvaluatorName", eval_circuit_id_t(0));
		return "Eval " + std::to_string(evaluatorId) + " (No Circuit)";
	}
	auto circuit = circuitManager.getCircuit(circuitId.value());
	if (!circuit) {
		logError("Circuit with id {} not found", "Evaluator::getEvaluatorName", circuitId.value());
		return "Eval " + std::to_string(evaluatorId) + " (Invalid Circuit)";
	}
	return "Eval " + std::to_string(evaluatorId) + " (" + circuit->getCircuitNameNumber() + ")";
}

void Evaluator::makeEdit(DifferenceSharedPtr difference, circuit_id_t circuitId) {
	evaluatorInternal->startEdit();
	for (const Difference::Modification& modification : difference->getModifications()) {
		const auto& [modificationType, modificationData] = modification;
		switch (modificationType) {
		case Difference::ModificationType::REMOVED_BLOCK: {
			const auto& [position, orientation, blockType] = std::get<Difference::block_modification_t>(modificationData);
			evaluatorInternal->removeBlock(position, orientation, blockType);
			break;
		}
		case Difference::ModificationType::PLACE_BLOCK: {
			const auto& [position, orientation, blockType] = std::get<Difference::block_modification_t>(modificationData);
			evaluatorInternal->addBlock(position, orientation, blockType);
			break;
		}
		case Difference::ModificationType::MOVE_BLOCK: {
			const auto& [curPosition, curOrientation, newPosition, newOrientation, finalMove] = std::get<Difference::move_modification_t>(modificationData);
			evaluatorInternal->moveBlock(curPosition, curOrientation, newPosition, newOrientation);
			break;
		}
		case Difference::ModificationType::REMOVED_CONNECTION: {
			const auto& [outputBlockPosition, outputPosition, inputBlockPosition, inputPosition] = std::get<Difference::connection_modification_t>(modificationData);
			evaluatorInternal->removeConnection(outputBlockPosition, outputPosition, inputBlockPosition, inputPosition);
			break;
		}
		case Difference::ModificationType::CREATED_CONNECTION: {
			const auto& [outputBlockPosition, outputPosition, inputBlockPosition, inputPosition] = std::get<Difference::connection_modification_t>(modificationData);
			evaluatorInternal->createConnection(outputBlockPosition, outputPosition, inputBlockPosition, inputPosition);
			break;
		}
		}
	}
	evaluatorInternal->endEdit();
	evalLogicSimulator.processEdits();
}

nlohmann::json Evaluator::dumpState() const /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json stateJson;
	stateJson["evaluatorId"] = evaluatorId.get();
	stateJson["interCircuitConnections"] = nlohmann::json::array();
	stateJson["dirtySimulatorIds"] = nlohmann::json::array();
	stateJson["dirtyMiddleIds"] = nlohmann::json::array();
	stateJson["dirtyNodes"] = nlohmann::json::array();
	stateJson["pinSimulatorIdToEvalPositionMap"] = nlohmann::json::object();
	stateJson["circuitNodeToBlockTypeMap"] = nlohmann::json::object();

	return stateJson;
}
