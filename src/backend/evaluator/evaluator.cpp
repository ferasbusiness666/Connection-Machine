#include "evaluator.h"

#include "backend/circuit/circuitManager.h"
#include "evaluatorInternal.h"
#include "util/evalLayerState.h"

#ifdef TRACY_PROFILER
#include <tracy/Tracy.hpp>
#endif

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
	evalConfig(dataUpdateEventManager, evaluatorId),
	circuitId(circuitId),
	evaluatorInternal(std::make_unique<EvaluatorInternal>(circuitId, circuitManager)),
	evalLogicSimulator(evalConfig, circuitManager.getBlockDataManager(), *evaluatorInternal)
{
	const Circuit* circuit = circuitManager.getCircuit(circuitId).get();
	assert(circuit);

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
	auto iter2 = evalLogicSimulator.getGateIdMapping().find(evaluatorInternal->mapFromAddressToBottomConnectionPoint(address).gateId);
	if (iter2 == evalLogicSimulator.getGateIdMapping().end()) {
		logError("Evaluator::getState(const Address& address) failed. Failed to get sim id");
		return logic_state_t::UNDEFINED;
	}
	return evalLogicSimulator.getState(iter2->second);
}

void Evaluator::setState(const Address& address, logic_state_t state) {
	auto iter2 = evalLogicSimulator.getGateIdMapping().find(evaluatorInternal->mapFromAddressToBottomConnectionPoint(address).gateId);
	if (iter2 == evalLogicSimulator.getGateIdMapping().end()) {
		logError("Evaluator::getState(const Address& address) failed. Failed to get sim id");
		return;
	}
	evalLogicSimulator.setState(iter2->second, state);
}

std::variant<simulator_id_t, std::vector<simulator_id_t>> Evaluator::getVirtualConnectionSimulatorId(const Address& address, virtual_connection_id_t virtualConnectionId) const {
	// if (virtualConnectionId != 0) return 0;
	auto iter2 = evalLogicSimulator.getGateIdMapping().find(evaluatorInternal->mapFromAddressToBottomConnectionPoint(address).gateId);
	if (iter2 == evalLogicSimulator.getGateIdMapping().end()) {
		logError("Evaluator::getBlockSimulatorId(const Address& address) failed. Failed to get sim id.", "Evaluator::getVirtualConnectionSimulatorId");
		return 0;
	}
	return iter2->second;
}

std::variant<simulator_id_t, std::vector<simulator_id_t>> Evaluator::getPinSimulatorId(const Address& address) const {
	EvalConnectionPoint evalConnectionPoint = evaluatorInternal->mapFromAddressToBottomConnectionPoint(address);
	if (evalConnectionPoint.isNull()) {
		logError("Evaluator::mapFromAddressToBottomConnectionPoint(const Address& address) failed. Failed to get bottom connection point.", "Evaluator::getPinSimulatorId");
		return 0;
	}
	const EvalLayerState& evalLayerState = evaluatorInternal->getLayerRunner().getOutputLayer();
	const EvalGate* evalGate = evalLayerState.getGate(evalConnectionPoint.gateId);
	auto connectionsIter = evalGate->connections.find(evalConnectionPoint.connectionEndId);
	eval_gate_id evalGateIdToReadState = evalConnectionPoint.gateId;
	if (connectionsIter != evalGate->connections.end() && connectionsIter->second.size() == 1) {
		const EvalGate* otherEvalGate = evalLayerState.getGate(connectionsIter->second.begin()->gateId);
		if (
			otherEvalGate->type == getEvalGateType(BlockType::JUNCTION) ||
			otherEvalGate->type == getEvalGateType(BlockType::JUNCTION_H) ||
			otherEvalGate->type == getEvalGateType(BlockType::JUNCTION_L) ||
			otherEvalGate->type == getEvalGateType(BlockType::JUNCTION_X)
		) {
			evalGateIdToReadState = otherEvalGate->gateId;
		}
	}
	auto iter2 = evalLogicSimulator.getGateIdMapping().find(evalGateIdToReadState);
	if (iter2 == evalLogicSimulator.getGateIdMapping().end()) {
		logError("Evaluator::getBlockSimulatorId(const Address& address) failed. Failed to get sim id.", "Evaluator::getPinSimulatorId");
		return 0;
	}
	return iter2->second;
}

std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> Evaluator::getVirtualConnectionSimulatorIds(const Address& addressOrigin, const std::vector<std::pair<Position, virtual_connection_id_t>>& virtualConnections) const {
	std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> output;
	for (std::pair<Position, virtual_connection_id_t> virtualConnection : virtualConnections) {
		Address address = addressOrigin;
		address.addBlockId(virtualConnection.first);
		output.push_back(getVirtualConnectionSimulatorId(address, virtualConnection.second));
	}
	return output;
}

std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> Evaluator::getPinSimulatorIds(const Address& addressOrigin, const std::vector<Position>& positions) const {
	std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> output;
	for (Position position : positions) {
		Address address = addressOrigin;
		address.addBlockId(position);
		output.push_back(getPinSimulatorId(address));
	}
	return output;
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
