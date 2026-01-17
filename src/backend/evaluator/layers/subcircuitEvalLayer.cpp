#include "subcircuitEvalLayer.h"
#include "evalLayerState.h"
#include "backend/circuit/circuitManager.h"
#include "backend/evaluator/evaluator.h"
#include "backend/evaluator/evaluatorInternal.h"

void SubcircuitEvalLayer::run() {
	for (auto iter : currentState.getRemovedConnections()) {
		EvalConnection connection = iter.first;
		auto subcircuitDataAIter = subcircuits.find(connection.connectionPointA.gateId);
		if (subcircuitDataAIter != subcircuits.end()) {
			const Circuit* circuit = circuitManager.getCircuit(subcircuitDataAIter->second.circuitId).get();
			assert(circuit);
			const EvaluatorInternal& evaluatorInternalval = circuit->getEvaluator().getEvaluatorInternal();
			auto internalPointMappingIter = evaluatorInternalval.getPortToInternalPointMapping().find(connection.connectionPointA.connectionEndId);
			assert(internalPointMappingIter != evaluatorInternalval.getPortToInternalPointMapping().end());
			if (!internalPointMappingIter->second.connectionPoint.has_value()) continue;
			EvalConnectionPoint internalBottomPoint = evaluatorInternalval.mapFromTopConnectionPointToBottomConnectionPoint(
				internalPointMappingIter->second.connectionPoint.value()
			);
			auto otherSimulatorToThisSimulatorIdMappingIter = subcircuitDataAIter->second.otherSimulatorToThisSimulatorIdMapping.find(
				internalBottomPoint.gateId
			);
			assert(otherSimulatorToThisSimulatorIdMappingIter != subcircuitDataAIter->second.otherSimulatorToThisSimulatorIdMapping.end());
			connection.connectionPointA = EvalConnectionPoint(
				otherSimulatorToThisSimulatorIdMappingIter->second,
				internalBottomPoint.connectionEndId
			);
		}
		auto subcircuitDataBIter = subcircuits.find(connection.connectionPointB.gateId);
		if (subcircuitDataBIter != subcircuits.end()) {
			const Circuit* circuit = circuitManager.getCircuit(subcircuitDataBIter->second.circuitId).get();
			assert(circuit);
			const EvaluatorInternal& evaluatorInternalval = circuit->getEvaluator().getEvaluatorInternal();
			auto internalPointMappingIter = evaluatorInternalval.getPortToInternalPointMapping().find(connection.connectionPointB.connectionEndId);
			assert(internalPointMappingIter != evaluatorInternalval.getPortToInternalPointMapping().end());
			if (!internalPointMappingIter->second.connectionPoint.has_value()) continue;
			EvalConnectionPoint internalBottomPoint = evaluatorInternalval.mapFromTopConnectionPointToBottomConnectionPoint(
				internalPointMappingIter->second.connectionPoint.value()
			);
			auto otherSimulatorToThisSimulatorIdMappingIter = subcircuitDataBIter->second.otherSimulatorToThisSimulatorIdMapping.find(
				internalBottomPoint.gateId
			);
			assert(otherSimulatorToThisSimulatorIdMappingIter != subcircuitDataBIter->second.otherSimulatorToThisSimulatorIdMapping.end());
			connection.connectionPointB = EvalConnectionPoint(
				otherSimulatorToThisSimulatorIdMappingIter->second,
				internalBottomPoint.connectionEndId
			);
		}
		nextState.removeConnection(connection, iter.second);
	}
	for (auto iter : currentState.getRemovedGates()) {
		auto subcircuitDataIter = subcircuits.find(iter.first);
		if (subcircuitDataIter == subcircuits.end()) {
			auto iterPair = nextState.getGateIdReverseRemapping().equal_range(iter.first);
			for (auto reverseRemappingiter = iterPair.first; reverseRemappingiter != iterPair.second; reverseRemappingiter++) {
				nextState.getGateIdRemapping().erase(reverseRemappingiter->second);
			}
			nextState.getGateIdReverseRemapping().erase(iterPair.first, iterPair.second);
			nextState.removeGate(iter.first);
			continue;
		}
		const Circuit* circuit = circuitManager.getCircuit(subcircuitDataIter->second.circuitId).get();
		assert(circuit);
		circuit->getEvaluator().getEvaluatorInternal().removeEvaluator(*this);
		const EvalLayerState& evalLayerState = subcircuitDataIter->second.outputEvalLayer;
		for (std::pair<eval_gate_id, EvalGate> pair : evalLayerState.getGates()) {
			eval_gate_id thisGateId = subcircuitDataIter->second.otherSimulatorToThisSimulatorIdMapping.at(pair.second.gateId);
			for (std::pair<connection_end_id_t, std::unordered_set<EvalConnectionPoint>> connectionsPair : pair.second.connections) {
				for (EvalConnectionPoint otherConnectionPoint : connectionsPair.second) {
					// need to add some logic to not double make connections
					if (
						(otherConnectionPoint.gateId > pair.second.gateId) ||
						(otherConnectionPoint.gateId == pair.second.gateId && otherConnectionPoint.connectionEndId.get() > connectionsPair.first.get())
					) {
						eval_gate_id otherGateId = subcircuitDataIter->second.otherSimulatorToThisSimulatorIdMapping.at(otherConnectionPoint.gateId);
						nextState.removeConnection(EvalConnection(
							EvalConnectionPoint(thisGateId, connectionsPair.first),
							EvalConnectionPoint(otherGateId, otherConnectionPoint.connectionEndId)
						));
					}
				}
			}
		}
		for (std::pair<eval_gate_id, EvalGate> pair : evalLayerState.getGates()) {
			eval_gate_id thisGateId = subcircuitDataIter->second.otherSimulatorToThisSimulatorIdMapping.at(pair.second.gateId);
			nextState.removeGate(thisGateId);
		}
		for (std::pair<connection_end_id_t, EvaluatorInternal::InternalPointData> pair : circuit->getEvaluator().getEvaluatorInternal().getPortToInternalPointMapping()) {
			auto connectionPointRemappingIter = nextState.getConnectionPointRemapping().find(EvalConnectionPoint(iter.first, pair.first));
			if (connectionPointRemappingIter == nextState.getConnectionPointRemapping().end()) continue;
			auto iterPair = nextState.getConnectionPointReverseRemapping().equal_range(connectionPointRemappingIter->second);
			for (auto reverseRemappingIter = iterPair.first; reverseRemappingIter != iterPair.second; reverseRemappingIter++) {
				if (reverseRemappingIter->second == EvalConnectionPoint(iter.first, pair.first)) {
					nextState.getConnectionPointReverseRemapping().erase(reverseRemappingIter);
					break;
				}
			}
			nextState.getConnectionPointRemapping().erase(connectionPointRemappingIter);
		}
		subcircuits.erase(subcircuitDataIter);
	}
	for (auto iter : currentState.getAddedGates()) {
		circuit_id_t circuitId = circuitManager.getCircuitBlockDataManager().getCircuitId(getBlockType(iter.second));
		if (circuitId == 0) {
			nextState.getGateIdRemapping().emplace(iter.first, iter.first);
			nextState.getGateIdReverseRemapping().emplace(iter.first, iter.first);
			nextState.addGate(iter.first, iter.second);
			continue;
		}
		const Circuit* circuit = circuitManager.getCircuit(circuitId).get();
		assert(circuit);
		circuit->getEvaluator().getEvaluatorInternal().addEvaluator(*this);
		const EvaluatorInternal& evaluatorInternal = circuit->getEvaluator().getEvaluatorInternal();
		const EvalLayerState& evalLayerState = evaluatorInternal.getLayerRunner().getOutputLayer();
		auto subcircuitsPair = subcircuits.try_emplace(iter.first, circuitId, evalLayerState);
		for (std::pair<eval_gate_id, EvalGate> pair : evalLayerState.getGates()) {
			eval_gate_id gateId = nextState.getUnusedEvalGateId();
			subcircuitsPair.first->second.otherSimulatorToThisSimulatorIdMapping.emplace(pair.second.gateId, gateId);
			subcircuitsPair.first->second.thisSimulatorIdMappingToOtherSimulator.emplace(gateId, pair.second.gateId);
			nextState.addGate(gateId, pair.second.type);
		}
		for (std::pair<eval_gate_id, EvalGate> pair : evalLayerState.getGates()) {
			eval_gate_id thisGateId = subcircuitsPair.first->second.otherSimulatorToThisSimulatorIdMapping.at(pair.second.gateId);
			for (std::pair<connection_end_id_t, std::unordered_set<EvalConnectionPoint>> connectionsPair : pair.second.connections) {
				for (EvalConnectionPoint otherConnectionPoint : connectionsPair.second) {
					// need to add some logic to not double make connections
					if (
						(otherConnectionPoint.gateId > pair.second.gateId) ||
						(otherConnectionPoint.gateId == pair.second.gateId && otherConnectionPoint.connectionEndId.get() > connectionsPair.first.get())
					) {
						eval_gate_id otherGateId = subcircuitsPair.first->second.otherSimulatorToThisSimulatorIdMapping.at(otherConnectionPoint.gateId);
						unsigned int weight = evalLayerState.getConnectionWeight(EvalConnection(
							EvalConnectionPoint(pair.first, connectionsPair.first),
							otherConnectionPoint
						));
						nextState.addConnection(EvalConnection(
							EvalConnectionPoint(thisGateId, connectionsPair.first),
							EvalConnectionPoint(otherGateId, otherConnectionPoint.connectionEndId)
						), weight);
					}
				}
			}
		}
		for (const std::pair<connection_end_id_t, EvaluatorInternal::InternalPointData>& pair : evaluatorInternal.getPortToInternalPointMapping()) {
			if (pair.second.connectionPoint) {
				const EvalConnectionPoint& bottomConnectionPoint = evaluatorInternal.mapFromTopConnectionPointToBottomConnectionPoint(
					pair.second.connectionPoint.value()
				);
				eval_gate_id thisGateId = subcircuitsPair.first->second.otherSimulatorToThisSimulatorIdMapping.at(bottomConnectionPoint.gateId);
				nextState.getConnectionPointRemapping().emplace(
					EvalConnectionPoint(iter.first, pair.first),
					EvalConnectionPoint(thisGateId, pair.second.connectionPoint->connectionEndId)
				);
				nextState.getConnectionPointReverseRemapping().emplace(
					EvalConnectionPoint(thisGateId, pair.second.connectionPoint->connectionEndId),
					EvalConnectionPoint(iter.first, pair.first)
				);
			}
		}
	}
	for (auto iter : currentState.getAddedConnections()) {
		EvalConnection connection = iter.first;
		auto subcircuitDataAIter = subcircuits.find(connection.connectionPointA.gateId);
		if (subcircuitDataAIter != subcircuits.end()) {
			const Circuit* circuit = circuitManager.getCircuit(subcircuitDataAIter->second.circuitId).get();
			assert(circuit);
			const EvaluatorInternal& evaluatorInternalval = circuit->getEvaluator().getEvaluatorInternal();
			auto internalPointMappingIter = evaluatorInternalval.getPortToInternalPointMapping().find(connection.connectionPointA.connectionEndId);
			assert(internalPointMappingIter != evaluatorInternalval.getPortToInternalPointMapping().end());
			if (!internalPointMappingIter->second.connectionPoint.has_value()) continue;
			EvalConnectionPoint internalBottomPoint = evaluatorInternalval.mapFromTopConnectionPointToBottomConnectionPoint(
				internalPointMappingIter->second.connectionPoint.value()
			);
			auto otherSimulatorToThisSimulatorIdMappingIter = subcircuitDataAIter->second.otherSimulatorToThisSimulatorIdMapping.find(
				internalBottomPoint.gateId
			);
			assert(otherSimulatorToThisSimulatorIdMappingIter != subcircuitDataAIter->second.otherSimulatorToThisSimulatorIdMapping.end());
			connection.connectionPointA = EvalConnectionPoint(
				otherSimulatorToThisSimulatorIdMappingIter->second,
				internalBottomPoint.connectionEndId
			);
		}
		auto subcircuitDataBIter = subcircuits.find(connection.connectionPointB.gateId);
		if (subcircuitDataBIter != subcircuits.end()) {
			const Circuit* circuit = circuitManager.getCircuit(subcircuitDataBIter->second.circuitId).get();
			assert(circuit);
			const EvaluatorInternal& evaluatorInternalval = circuit->getEvaluator().getEvaluatorInternal();
			auto internalPointMappingIter = evaluatorInternalval.getPortToInternalPointMapping().find(connection.connectionPointB.connectionEndId);
			assert(internalPointMappingIter != evaluatorInternalval.getPortToInternalPointMapping().end());
			if (!internalPointMappingIter->second.connectionPoint.has_value()) continue;
			EvalConnectionPoint internalBottomPoint = evaluatorInternalval.mapFromTopConnectionPointToBottomConnectionPoint(
				internalPointMappingIter->second.connectionPoint.value()
			);
			auto otherSimulatorToThisSimulatorIdMappingIter = subcircuitDataBIter->second.otherSimulatorToThisSimulatorIdMapping.find(
				internalBottomPoint.gateId
			);
			assert(otherSimulatorToThisSimulatorIdMappingIter != subcircuitDataBIter->second.otherSimulatorToThisSimulatorIdMapping.end());
			connection.connectionPointB = EvalConnectionPoint(
				otherSimulatorToThisSimulatorIdMappingIter->second,
				internalBottomPoint.connectionEndId
			);
		}
		nextState.addConnection(connection, iter.second);
	}
}

void SubcircuitEvalLayer::processICEdits(circuit_id_t circuitId, const std::vector<connection_end_id_t>& updatedPortIds) {
	evaluator.startEdit();
	const Circuit* circuit = circuitManager.getCircuit(circuitId).get();
	assert(circuit);
	const EvaluatorInternal& evaluatorInternal = circuit->getEvaluator().getEvaluatorInternal();
	for (std::pair<const eval_gate_id, SubcircuitData>& subcircuitsPair : subcircuits) {
		if (subcircuitsPair.second.circuitId == circuitId) {
			for (connection_end_id_t connectionEndId : updatedPortIds) {
				auto portMappingIter = evaluatorInternal.getPortToInternalPointMapping().find(connectionEndId);
				if (portMappingIter == evaluatorInternal.getPortToInternalPointMapping().end()) {
					auto connectionPointRemappingIter = nextState.getConnectionPointRemapping().find(EvalConnectionPoint(subcircuitsPair.first, connectionEndId));
					if (connectionPointRemappingIter == nextState.getConnectionPointRemapping().end()) continue;
					auto iterPair = nextState.getConnectionPointReverseRemapping().equal_range(connectionPointRemappingIter->second);
					for (auto iter = iterPair.first; iter != iterPair.second; iter++) {
						if (iter->second == EvalConnectionPoint(subcircuitsPair.first, connectionEndId)) {
							nextState.getConnectionPointReverseRemapping().erase(iter);
							break;
						}
					}
					nextState.getConnectionPointRemapping().erase(connectionPointRemappingIter);
				} else if (portMappingIter->second.connectionPoint) {
					const EvalConnectionPoint& bottomConnectionPoint = evaluatorInternal.mapFromTopConnectionPointToBottomConnectionPoint(
						portMappingIter->second.connectionPoint.value()
					);
					eval_gate_id thisGateId = subcircuitsPair.second.otherSimulatorToThisSimulatorIdMapping.at(bottomConnectionPoint.gateId);
					nextState.getConnectionPointRemapping().emplace(
						EvalConnectionPoint(subcircuitsPair.first, connectionEndId),
						EvalConnectionPoint(thisGateId, portMappingIter->second.connectionPoint->connectionEndId)
					);
					nextState.getConnectionPointReverseRemapping().emplace(
						EvalConnectionPoint(thisGateId, portMappingIter->second.connectionPoint->connectionEndId),
						EvalConnectionPoint(subcircuitsPair.first, connectionEndId)
					);
					const EvalGate* evalGate = currentState.getGate(subcircuitsPair.first);
					auto connectionsIter = evalGate->connections.find(connectionEndId);
					if (connectionsIter != evalGate->connections.end()) {
						for (EvalConnectionPoint otherConnectionPoint : connectionsIter->second) {
							nextState.addConnection(EvalConnection(
								EvalConnectionPoint(thisGateId, portMappingIter->second.connectionPoint->connectionEndId),
								otherConnectionPoint
							));
						}
					}
				} else {
					auto connectionPointRemappingIter = nextState.getConnectionPointRemapping().find(EvalConnectionPoint(subcircuitsPair.first, connectionEndId));
					if (connectionPointRemappingIter == nextState.getConnectionPointRemapping().end()) continue;
					const EvalGate* evalGate = currentState.getGate(connectionPointRemappingIter->second.gateId);
					auto connectionsIter = evalGate->connections.find(connectionPointRemappingIter->second.connectionEndId);
					if (connectionsIter != evalGate->connections.end()) {
						for (EvalConnectionPoint otherConnectionPoint : connectionsIter->second) {
							if (!subcircuitsPair.second.thisSimulatorIdMappingToOtherSimulator.contains(otherConnectionPoint.gateId)) {
								nextState.removeConnection(EvalConnection(
									EvalConnectionPoint(connectionPointRemappingIter->second.gateId, portMappingIter->second.connectionPoint->connectionEndId),
									otherConnectionPoint
								));
							}
						}
					}
					auto iterPair = nextState.getConnectionPointReverseRemapping().equal_range(connectionPointRemappingIter->second);
					for (auto iter = iterPair.first; iter != iterPair.second; iter++) {
						if (iter->second == EvalConnectionPoint(subcircuitsPair.first, connectionEndId)) {
							nextState.getConnectionPointReverseRemapping().erase(iter);
							break;
						}
					}
					nextState.getConnectionPointRemapping().erase(connectionPointRemappingIter);
				}
			}
		}
	}
	evaluator.endEdit();
}
