#include "evaluator.h"

#include "backend/circuit/circuitManager.h"
#include "evaluatorInternal.h"

#ifdef TRACY_PROFILER
#include <tracy/Tracy.hpp>
#endif

Evaluator::~Evaluator() = default;

Evaluator::Evaluator(
	evaluator_id_t evaluatorId,
	CircuitManager& circuitManager,
	BlockDataManager& blockDataManager,
	CircuitBlockDataManager& circuitBlockDataManager,
	circuit_id_t circuitId,
	DataUpdateEventManager& dataUpdateEventManager
) :
	evaluatorId(evaluatorId),
	circuitManager(circuitManager),
	blockDataManager(blockDataManager),
	circuitBlockDataManager(circuitBlockDataManager),
	dataUpdateEventManager(dataUpdateEventManager),
	receiver(dataUpdateEventManager),
	evalConfig(dataUpdateEventManager, evaluatorId),
	circuitId(circuitId),
	evaluatorInternal(std::make_unique<EvaluatorInternal>(blockDataManager, circuitBlockDataManager))
{
	const auto circuit = circuitManager.getCircuit(circuitId);
	if (!circuit) {
		logError("Circuit with ID {} not found", "Evaluator::Evaluator", circuitId);
		return;
	}
	logInfo("Creating Evaluator with ID {} for Circuit ID {}", "Evaluator", evaluatorId, circuitId);
	const BlockContainer& blockContainer = circuit->getBlockContainer();
	const Difference difference = blockContainer.getCreationDifference();
	// receiver.linkFunction("circuitBlockDataConnectionPositionRemove", std::bind(&Evaluator::removeCircuitIO, this, std::placeholders::_1));
	// receiver.linkFunction("circuitBlockDataConnectionPositionSet", std::bind(&Evaluator::setCircuitIO, this, std::placeholders::_1, false));
	// receiver.linkFunction("blockDataPortBitConfigurationSet", std::bind(&Evaluator::setCircuitIO, this, std::placeholders::_1, true)); // TODO: this only works for ICs, needs to work for all blocks

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
}

nlohmann::json Evaluator::dumpState() const /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json stateJson;
	stateJson["evaluatorId"] = evaluatorId.get();
	stateJson["evalConfig"] = evalConfig.dumpState();
	stateJson["interCircuitConnections"] = nlohmann::json::array();
	stateJson["dirtySimulatorIds"] = nlohmann::json::array();
	stateJson["dirtyMiddleIds"] = nlohmann::json::array();
	stateJson["dirtyNodes"] = nlohmann::json::array();
	stateJson["pinSimulatorIdToEvalPositionMap"] = nlohmann::json::object();
	stateJson["circuitNodeToBlockTypeMap"] = nlohmann::json::object();

	return stateJson;
}
