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
	evaluatorInternal(std::make_unique<EvaluatorInternal>(blockDataManager, circuitBlockDataManager)),
	evalLogicSimulator(evalConfig, blockDataManager, *evaluatorInternal)
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
	evalLogicSimulator.makeEdit();
}

logic_state_t Evaluator::getState(const Address& address) const {
	if (address.size() != 1) {
		logError("Evaluator::getState(const Address& address) failed. Eval not working with ICs!");
		return logic_state_t::UNDEFINED;
	}
	auto iter = evaluatorInternal->getPositionRemapping().find(address.getPosition(0));
	if (iter == evaluatorInternal->getPositionRemapping().end()) {
		logError("Evaluator::getState(const Address& address) failed. Failed to get eval id");
		return logic_state_t::UNDEFINED;
	}
	auto iter2 = evalLogicSimulator.getGateIdMapping().find(evaluatorInternal->getLayerRunner().getMappedEvalConnectionPoint(EvalConnectionPoint(iter->second.first, 1)).gateId);
	if (iter2 == evalLogicSimulator.getGateIdMapping().end()) {
		logError("Evaluator::getState(const Address& address) failed. Failed to get sim id");
		return logic_state_t::UNDEFINED;
	}
	return evalLogicSimulator.getState(iter2->second);
}

void Evaluator::setState(const Address& address, logic_state_t state) {
	if (address.size() != 1) {
		logError("Evaluator::getState(const Address& address) failed. Eval not working with ICs!");
		return;
	}
	auto iter = evaluatorInternal->getPositionRemapping().find(address.getPosition(0));
	if (iter == evaluatorInternal->getPositionRemapping().end()) {
		logError("Evaluator::getState(const Address& address) failed. Failed to get eval id");
		return;
	}
	auto iter2 = evalLogicSimulator.getGateIdMapping().find(evaluatorInternal->getLayerRunner().getMappedEvalConnectionPoint(EvalConnectionPoint(iter->second.first, 1)).gateId);
	if (iter2 == evalLogicSimulator.getGateIdMapping().end()) {
		logError("Evaluator::getState(const Address& address) failed. Failed to get sim id");
		return;
	}
	evalLogicSimulator.setState(iter2->second, state);
}

simulator_id_t Evaluator::getBlockSimulatorId(const Address& address) const {
	if (address.size() != 1) {
		logError("Evaluator::getBlockSimulatorId(const Address& address) failed. Eval not working with ICs!");
		return 0;
	}
	auto iter = evaluatorInternal->getPositionRemapping().find(address.getPosition(0));
	if (iter == evaluatorInternal->getPositionRemapping().end()) {
		logError("Evaluator::getBlockSimulatorId(const Address& address) failed. Failed to get eval id");
		return 0;
	}
	auto iter2 = evalLogicSimulator.getGateIdMapping().find(evaluatorInternal->getLayerRunner().getMappedEvalConnectionPoint(EvalConnectionPoint(iter->second.first, 1)).gateId);
	if (iter2 == evalLogicSimulator.getGateIdMapping().end()) {
		logError("Evaluator::getBlockSimulatorId(const Address& address) failed. Failed to get sim id");
		return 0;
	}
	return iter2->second;
}

std::vector<simulator_id_t> Evaluator::getBlockSimulatorIds(const Address& addressOrigin, const std::vector<Position>& positions) const {
	if (addressOrigin.size() != 0) {
		logError("Evaluator::getBlockSimulatorId(const Address& address) failed. Eval not working with ICs!");
		return { };
	}
	std::vector<simulator_id_t> simIds;
	for (auto pos : positions) {
		Address address(addressOrigin);
		address.nestPosition(pos);
		simIds.push_back(getBlockSimulatorId(address));
	}
	return simIds;
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
