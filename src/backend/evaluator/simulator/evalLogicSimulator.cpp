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
			if (pair.second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) continue;
			std::optional<simulator_id_t> stateIndex = logicSimulator.getOutputPortId(mappingPair.second, pair.first);
			if (!stateIndex) {
				logError("std::optional<simulator_id_t> stateIndex = logicSimulator.getOutputPortId(mappingPair.second, pair.first); Failed", "EvalLogicSimulator::makeEdit");
				continue;
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
				// logInfo("mapping update {}, {}", "", portPos, mappingPair.second);
				simulatorMappingUpdates.emplace_back(portPos, mappingPair.second);
				simulatorMappingUpdates.emplace_back(portPos, 1, mappingPair.second);
			}
		}
	}
	for (auto iter : simulatorMappingUpdateListeners) {
		iter.second.callback(simulatorMappingUpdates);
	}
}

std::optional<simulator_id_t> EvalLogicSimulator::getOutputPortId(eval_gate_id gateId, connection_end_id_t portId) const {
	auto gateIdIter = gateIdMapping.find(gateId);
	if (gateIdIter == gateIdMapping.end()) return std::nullopt;
	return logicSimulator.getOutputPortId(gateIdIter->second, portId);
}

void EvalLogicSimulator::connectListener(void* object, const Address& address, SimulatorMappingUpdateListenerFunction func) {
	simulatorMappingUpdateListeners.try_emplace(object, 0, func);
	// std::optional<eval_circuit_id_t> evalCircuitId = evalCircuitContainer.traverseToTopLevelIC(address);
	// if (!evalCircuitId) {
	// 	logError("Failed to connect listener for address {}: No top-level IC found", "Evaluator::connectListener", address.toString());
	// 	return;
	// }
	// listeners[object] = { evalCircuitId.value(), func };
	// std::unordered_map<eval_circuit_id_t, std::vector<SimulatorMappingUpdate>> simulatorMappingUpdates;
	// auto evalCircuit = evalCircuitContainer.getCircuit(evalCircuitId.value());
	// if (!evalCircuit) {
	// 	logError("Failed to get eval circuit for ID {}", "Evaluator::connectListener", evalCircuitId.value());
	// 	return;
	// }
	// evalCircuit->forEachNode([this, evalCircuitId](Position pos, const CircuitNode& node) { this->dirtyBlockAt(pos, evalCircuitId.value()); });
	// processDirtyNodes();
}
