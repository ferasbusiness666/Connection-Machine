#include "evalLogicSimulator.h"

#include "backend/blockData/blockDataManager.h"
#include "backend/evaluator/layers/layerRunner.h"
#include "backend/evaluator/util/evalLayerState.h"
#include "backend/evaluator/evaluatorInternal.h"

EvalLogicSimulator::EvalLogicSimulator(EvalConfig& evalConfig, const BlockDataManager& blockDataManager, const EvaluatorInternal& evaluatorInternal) :
	logicSimulator(evalConfig, dirtySimulatorIds), blockDataManager(blockDataManager), evaluatorInternal(evaluatorInternal) { }

void EvalLogicSimulator::makeEdit() {
	const EvalLayerState& evalLayerState = evaluatorInternal.getLayerRunner().getOutputLayer();
	{
		SimPauseGuard simPauseGuard(logicSimulator);
		for (auto iter = evalLayerState.getRemovedConnectionsBegin(); iter != evalLayerState.getRemovedConnectionsEnd(); ++iter) {
			auto gateAIdIter = gateIdMapping.find(iter->connectionPointA.gateId);
			if (gateAIdIter == gateIdMapping.end()) {
				logError("makeEdit remove connections gateIdMapping.find(iter->connectionPointA.gateId) failed. Gate id: {}", "EvalLogicSimulator::makeEdit", iter->connectionPointA.gateId);
				continue;
			}
			auto gateBIdIter = gateIdMapping.find(iter->connectionPointB.gateId);
			if (gateBIdIter == gateIdMapping.end()) {
				logError("makeEdit remove connections gateIdMapping.find(iter->connectionPointB.gateId) failed. Gate id: {}", "EvalLogicSimulator::makeEdit", iter->connectionPointB.gateId);
				continue;
			}
			logicSimulator.removeConnection(gateAIdIter->second, iter->connectionPointA.connectionEndId, gateBIdIter->second, iter->connectionPointB.connectionEndId);
		}
		for (auto iter = evalLayerState.getRemovedGatesBegin(); iter != evalLayerState.getRemovedGatesEnd(); ++iter) {
			auto gateIdIter = gateIdMapping.find(iter->get());
			if (gateIdIter == gateIdMapping.end()) {
				logError("makeEdit remove gate gateIdMapping.find(iter->connectionPointA.gateId) failed. Gate id: {}", "EvalLogicSimulator::makeEdit", iter->get());
				continue;
			}
			logicSimulator.removeGate(gateIdIter->second);
			gateIdMapping.erase(gateIdIter);
		}
		for (auto iter = evalLayerState.getAddedGatesBegin(); iter != evalLayerState.getAddedGatesEnd(); ++iter) {
			const EvalGate* evalGate = evalLayerState.getGate(iter->get());
			assert(evalGate);
			simulator_id_t simId = logicSimulator.addGate(getBlockType(evalGate->type));
			gateIdMapping.try_emplace(evalGate->gateId, simId);
		}
		for (auto iter = evalLayerState.getAddedConnectionsBegin(); iter != evalLayerState.getAddedConnectionsEnd(); ++iter) {
			auto gateAIdIter = gateIdMapping.find(iter->connectionPointA.gateId);
			if (gateAIdIter == gateIdMapping.end()) {
				logError("makeEdit add connections gateIdMapping.find(iter->connectionPointA.gateId) failed. Gate id: {}", "EvalLogicSimulator::makeEdit", iter->connectionPointA.gateId);
				continue;
			}
			auto gateBIdIter = gateIdMapping.find(iter->connectionPointB.gateId);
			if (gateBIdIter == gateIdMapping.end()) {
				logError("makeEdit add connections gateIdMapping.find(iter->connectionPointB.gateId) failed. Gate id: {}", "EvalLogicSimulator::makeEdit", iter->connectionPointB.gateId);
				continue;
			}
			logicSimulator.makeConnection(gateAIdIter->second, iter->connectionPointA.connectionEndId, gateBIdIter->second, iter->connectionPointB.connectionEndId);
		}
		logicSimulator.endEdit();
	}

	std::vector<SimulatorMappingUpdate> simulatorMappingUpdates;
	for (auto mappingPair : gateIdMapping) {
		const EvalGate* evalGate = evalLayerState.getGate(mappingPair.first);
		const BlockData* blockData = blockDataManager.getBlockData(getBlockType(evalGate->type));
		for (auto pair : blockData->getConnectionsSafe()) {
			if (pair.second.portType == BlockData::ConnectionData::PortType::INPUT) continue;
			std::optional<simulator_id_t> stateIndex = logicSimulator.getOutputPortId(mappingPair.second, pair.first);
			if (!stateIndex) {
				logError("std::optional<simulator_id_t> stateIndex = logicSimulator.getOutputPortId(mappingPair.second, pair.first); Failed", "EvalLogicSimulator::makeEdit");
				continue;
			}
			simulator_id_t pinSimId = mappingPair.second;
			auto connectionsIter = evalGate->connections.find(pair.first);
			if (connectionsIter != evalGate->connections.end() && connectionsIter->second.size() == 1) {
				const EvalGate* otherEvalGate = evalLayerState.getGate(connectionsIter->second.begin()->gateId);
				if (
					otherEvalGate->type == getEvalGateType(BlockType::JUNCTION) ||
					otherEvalGate->type == getEvalGateType(BlockType::JUNCTION_H) ||
					otherEvalGate->type == getEvalGateType(BlockType::JUNCTION_L) ||
					otherEvalGate->type == getEvalGateType(BlockType::JUNCTION_X)
				) {
					pinSimId = gateIdMapping.at(otherEvalGate->gateId);
				}
			}
			std::vector<EvalConnectionPoint> evalConnectionPoints = evaluatorInternal.getLayerRunner().getReversedMappedEvalConnectionPoint(EvalConnectionPoint(evalGate->gateId, pair.first));
			for (EvalConnectionPoint evalConnectionPoint : evalConnectionPoints) {
				evaluatorInternal.mapFromTopConnectionPointToAddress(evalConnectionPoint);
				auto iter = evaluatorInternal.getPositionReverseRemapping().find(evalConnectionPoint.gateId);
				if (iter == evaluatorInternal.getPositionReverseRemapping().end()) {
					logError("Could not find pos of gate wth eval id {}", "EvalLogicSimulator::makeEdit", evalConnectionPoint.gateId);
					continue;
				}
				Position portPos = iter->second.first + iter->second.second.transformVectorWithArea(pair.second.positionOnBlock, blockData->getSize());
				logInfo("mapping update {}, {}", "", portPos, mappingPair.second);
				simulatorMappingUpdates.emplace_back(portPos, pinSimId);
				simulatorMappingUpdates.emplace_back(iter->second.first, 0, stateIndex.value());
			}
		}
	}
	for (auto iter : simulatorMappingUpdateListeners) {
		iter.second.callback(simulatorMappingUpdates);
	}
}

logic_state_t EvalLogicSimulator::getState(const Address& address) const {
	auto iter2 = gateIdMapping.find(evaluatorInternal.mapFromAddressToBottomConnectionPoint(address).gateId);
	if (iter2 == gateIdMapping.end()) {
		logError("Failed to get sim id.", "EvalLogicSimulator::getState");
		return logic_state_t::UNDEFINED;
	}
	return getState(iter2->second);
}

void EvalLogicSimulator::setState(const Address& address, logic_state_t state) {
	auto iter2 = gateIdMapping.find(evaluatorInternal.mapFromAddressToBottomConnectionPoint(address).gateId);
	if (iter2 == gateIdMapping.end()) {
		logError("Failed to get sim id", "EvalLogicSimulator::setState");
		return;
	}
	setState(iter2->second, state);
}

std::optional<simulator_id_t> EvalLogicSimulator::getOutputPortId(eval_gate_id gateId, connection_end_id_t portId) const {
	auto gateIdIter = gateIdMapping.find(gateId);
	if (gateIdIter == gateIdMapping.end()) return std::nullopt;
	return logicSimulator.getOutputPortId(gateIdIter->second, portId);
}


std::variant<simulator_id_t, std::vector<simulator_id_t>> EvalLogicSimulator::getVirtualConnectionSimulatorId(const Address& address, virtual_connection_id_t virtualConnectionId) const {
	if (virtualConnectionId != 0) return 0;
	auto iter2 = gateIdMapping.find(evaluatorInternal.mapFromAddressToBottomConnectionPoint(address).gateId);
	if (iter2 == gateIdMapping.end()) {
		logError("Failed to get sim id.", "EvalLogicSimulator::getVirtualConnectionSimulatorId");
		return 0;
	}
	return iter2->second;
}

std::variant<simulator_id_t, std::vector<simulator_id_t>> EvalLogicSimulator::getPinSimulatorId(const Address& address) const {
	EvalConnectionPoint evalConnectionPoint = evaluatorInternal.mapFromAddressToBottomConnectionPoint(address);
	if (evalConnectionPoint.isNull()) {
		logError("Failed to get bottom connection point.", "EvalLogicSimulator::getPinSimulatorId");
		return 0;
	}
	const EvalLayerState& evalLayerState = evaluatorInternal.getLayerRunner().getOutputLayer();
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
	auto iter2 = gateIdMapping.find(evalGateIdToReadState);
	if (iter2 == gateIdMapping.end()) {
		logError("Failed to get sim id.", "EvalLogicSimulator::getPinSimulatorId");
		return 0;
	}
	return iter2->second;
}

std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> EvalLogicSimulator::getVirtualConnectionSimulatorIds(const Address& addressOrigin, const std::vector<std::pair<Position, virtual_connection_id_t>>& virtualConnections) const {
	std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> output;
	for (std::pair<Position, virtual_connection_id_t> virtualConnection : virtualConnections) {
		Address address = addressOrigin;
		address.addBlockId(virtualConnection.first);
		output.push_back(getVirtualConnectionSimulatorId(address, virtualConnection.second));
	}
	return output;
}

std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> EvalLogicSimulator::getPinSimulatorIds(const Address& addressOrigin, const std::vector<Position>& positions) const {
	std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> output;
	for (Position position : positions) {
		Address address = addressOrigin;
		address.addBlockId(position);
		output.push_back(getPinSimulatorId(address));
	}
	return output;
}

void EvalLogicSimulator::connectListener(void* object, const Address& address, SimulatorMappingUpdateListenerFunction func) {
	simulatorMappingUpdateListeners.try_emplace(object, 0, func);
	// std::optional<eval_circuit_id_t> evalCircuitId = evalCircuitContainer.traverseToTopLevelIC(address);
	// if (!evalCircuitId) {
	// 	logError("Failed to connect listener for address {}: No top-level IC found", "EvalLogicSimulator::connectListener", address.toString());
	// 	return;
	// }
	// listeners[object] = { evalCircuitId.value(), func };
	// std::unordered_map<eval_circuit_id_t, std::vector<SimulatorMappingUpdate>> simulatorMappingUpdates;
	// auto evalCircuit = evalCircuitContainer.getCircuit(evalCircuitId.value());
	// if (!evalCircuit) {
	// 	logError("Failed to get eval circuit for ID {}", "EvalLogicSimulator::connectListener", evalCircuitId.value());
	// 	return;
	// }
	// evalCircuit->forEachNode([this, evalCircuitId](Position pos, const CircuitNode& node) { this->dirtyBlockAt(pos, evalCircuitId.value()); });
	// processDirtyNodes();
}
