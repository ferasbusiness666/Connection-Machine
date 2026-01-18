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
			nextState.releaseUnusedEvalGateId(thisGateId);
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

std::vector<std::pair<eval_gate_id, circuit_id_t>> SubcircuitEvalLayer::getSubcircuits() const {
	std::vector<std::pair<eval_gate_id, circuit_id_t>> subcircuitPairs;
	for (const std::pair<eval_gate_id, SubcircuitData>& subcircuit : subcircuits) {
		subcircuitPairs.emplace_back(subcircuit.first, subcircuit.second.circuitId);
	}
	return subcircuitPairs;
}

EvalConnectionPoint SubcircuitEvalLayer::getMappedAddress(eval_gate_id gateId, const Address& address) const {
	auto subcircuitIter = subcircuits.find(gateId);
	if (subcircuitIter == subcircuits.end()) return EvalConnectionPoint::null();
	const Circuit* circuit = circuitManager.getCircuit(subcircuitIter->second.circuitId).get();
	assert(circuit);
	EvalConnectionPoint internalBottomPoint = circuit->getEvaluator().getEvaluatorInternal().mapFromAddressToBottomConnectionPoint(address.popTopPosition());
	if (internalBottomPoint.isNull()) return EvalConnectionPoint::null();
	auto otherSimulatorToThisSimulatorIdMappingIter = subcircuitIter->second.otherSimulatorToThisSimulatorIdMapping.find(internalBottomPoint.gateId);
	assert(otherSimulatorToThisSimulatorIdMappingIter != subcircuitIter->second.otherSimulatorToThisSimulatorIdMapping.end());
	return EvalConnectionPoint(
		otherSimulatorToThisSimulatorIdMappingIter->second,
		internalBottomPoint.connectionEndId
	);
}
std::vector<EvalConnectionPoint> SubcircuitEvalLayer::getReversedMappedConnectionPointsWithAddressMixed(const std::vector<EvalConnectionPoint>& connectionPoints, eval_gate_id gateId, const Address& address) const {
	auto subcircuitIter = subcircuits.find(gateId);
	if (subcircuitIter == subcircuits.end()) {
		if (address.size() == 0)
		return { };
	}
	std::vector<EvalConnectionPoint> subcircuitConnectionPoints;
	for (EvalConnectionPoint connectionPoint : connectionPoints) {
		auto iter = subcircuitIter->second.thisSimulatorIdMappingToOtherSimulator.find(connectionPoint.gateId);
		if (iter == subcircuitIter->second.thisSimulatorIdMappingToOtherSimulator.end()) continue;
		subcircuitConnectionPoints.push_back(EvalConnectionPoint(iter->second, connectionPoint.connectionEndId));
	}
	const Circuit* circuit = circuitManager.getCircuit(subcircuitIter->second.circuitId).get();
	assert(circuit);
	return circuit->getEvaluator().getEvaluatorInternal().mapFromBottomConnectionPointsToTopConnectionPointsMixed(subcircuitConnectionPoints, address);
}
std::vector<std::vector<EvalConnectionPoint>> SubcircuitEvalLayer::getReversedMappedConnectionPointGroupsWithAddress(const std::vector<std::vector<EvalConnectionPoint>>& connectionPoints, eval_gate_id gateId, const Address& address) const {
	auto subcircuitIter = subcircuits.find(gateId);
	if (subcircuitIter == subcircuits.end()) {
		if (address.size() == 0)
		return { };
	}
	std::vector<std::vector<EvalConnectionPoint>> subcircuitConnectionPoints;
	for (const std::vector<EvalConnectionPoint>& connectionPointVec : connectionPoints) {
		subcircuitConnectionPoints.push_back({});
		for (EvalConnectionPoint connectionPoint : connectionPointVec) {
			auto iter = subcircuitIter->second.thisSimulatorIdMappingToOtherSimulator.find(connectionPoint.gateId);
			if (iter == subcircuitIter->second.thisSimulatorIdMappingToOtherSimulator.end()) continue;
			subcircuitConnectionPoints.back().push_back(EvalConnectionPoint(iter->second, connectionPoint.connectionEndId));
		}
	}
	const Circuit* circuit = circuitManager.getCircuit(subcircuitIter->second.circuitId).get();
	assert(circuit);
	return circuit->getEvaluator().getEvaluatorInternal().mapFromBottomConnectionPointGroupsToTopConnectionPoints(subcircuitConnectionPoints, address);
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
				} else if (!portMappingIter->second.connectionPoint) {
					auto connectionPointRemappingIter = nextState.getConnectionPointRemapping().find(EvalConnectionPoint(subcircuitsPair.first, connectionEndId));
					if (connectionPointRemappingIter == nextState.getConnectionPointRemapping().end()) continue;
					const EvalGate* evalGate = nextState.getGate(connectionPointRemappingIter->second.gateId);
					auto connectionsIter = evalGate->connections.find(connectionPointRemappingIter->second.connectionEndId);
					if (connectionsIter != evalGate->connections.end()) {
						std::unordered_set<EvalConnectionPoint> otherConnectionPoints = connectionsIter->second;
						for (const EvalConnectionPoint& otherConnectionPoint : otherConnectionPoints) {
							if (!subcircuitsPair.second.thisSimulatorIdMappingToOtherSimulator.contains(otherConnectionPoint.gateId)) {
								nextState.removeConnection(EvalConnection(
									EvalConnectionPoint(connectionPointRemappingIter->second.gateId, connectionPointRemappingIter->second.connectionEndId),
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

			// internal updates
			for (auto iter : subcircuitsPair.second.outputEvalLayer.getRemovedConnections()) {
				eval_gate_id gateIdA = subcircuitsPair.second.otherSimulatorToThisSimulatorIdMapping.at(iter.first.connectionPointA.gateId);
				eval_gate_id gateIdB = subcircuitsPair.second.otherSimulatorToThisSimulatorIdMapping.at(iter.first.connectionPointB.gateId);
				nextState.removeConnection(EvalConnection(
					EvalConnectionPoint(gateIdA, iter.first.connectionPointA.connectionEndId),
					EvalConnectionPoint(gateIdB, iter.first.connectionPointB.connectionEndId)
				), iter.second);
			}
			for (auto iter : subcircuitsPair.second.outputEvalLayer.getRemovedGates()) {
				auto otherSimulatorToThisSimulatorIdMappingIter = subcircuitsPair.second.otherSimulatorToThisSimulatorIdMapping.find(iter.first);
				subcircuitsPair.second.thisSimulatorIdMappingToOtherSimulator.erase(otherSimulatorToThisSimulatorIdMappingIter->second);
				subcircuitsPair.second.otherSimulatorToThisSimulatorIdMapping.erase(otherSimulatorToThisSimulatorIdMappingIter);
				nextState.removeGate(otherSimulatorToThisSimulatorIdMappingIter->second);
				nextState.releaseUnusedEvalGateId(otherSimulatorToThisSimulatorIdMappingIter->second);
			}
			for (auto iter : subcircuitsPair.second.outputEvalLayer.getAddedGates()) {
				eval_gate_id gateId = nextState.getUnusedEvalGateId();
				subcircuitsPair.second.otherSimulatorToThisSimulatorIdMapping.emplace(iter.first, gateId);
				subcircuitsPair.second.thisSimulatorIdMappingToOtherSimulator.emplace(gateId, iter.first);
				nextState.addGate(gateId, iter.second);
			}
			for (auto iter : subcircuitsPair.second.outputEvalLayer.getAddedConnections()) {
				eval_gate_id gateIdA = subcircuitsPair.second.otherSimulatorToThisSimulatorIdMapping.at(iter.first.connectionPointA.gateId);
				eval_gate_id gateIdB = subcircuitsPair.second.otherSimulatorToThisSimulatorIdMapping.at(iter.first.connectionPointB.gateId);
				nextState.addConnection(EvalConnection(
					EvalConnectionPoint(gateIdA, iter.first.connectionPointA.connectionEndId),
					EvalConnectionPoint(gateIdB, iter.first.connectionPointB.connectionEndId)
				), iter.second);
			}

			// post port updates
			for (connection_end_id_t connectionEndId : updatedPortIds) {
				auto portMappingIter = evaluatorInternal.getPortToInternalPointMapping().find(connectionEndId);
				if (portMappingIter == evaluatorInternal.getPortToInternalPointMapping().end()) continue;
				if (!portMappingIter->second.connectionPoint) continue;
				const EvalConnectionPoint& bottomConnectionPoint = evaluatorInternal.mapFromTopConnectionPointToBottomConnectionPoint(
					portMappingIter->second.connectionPoint.value()
				);
				eval_gate_id thisGateId = subcircuitsPair.second.otherSimulatorToThisSimulatorIdMapping.at(bottomConnectionPoint.gateId);
				EvalConnectionPoint connectionPoint(thisGateId, portMappingIter->second.connectionPoint->connectionEndId);
				auto emplaceResult = nextState.getConnectionPointRemapping().emplace(
					EvalConnectionPoint(subcircuitsPair.first, connectionEndId),
					connectionPoint
				);
				if (emplaceResult.second) {
					nextState.getConnectionPointReverseRemapping().emplace(
						connectionPoint,
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
				} else if (emplaceResult.first->second != connectionPoint) {
					auto iterPair = nextState.getConnectionPointReverseRemapping().equal_range(emplaceResult.first->second);
					for (auto iter = iterPair.first; iter != iterPair.second; iter++) {
						if (iter->second == EvalConnectionPoint(subcircuitsPair.first, connectionEndId)) {
							nextState.getConnectionPointReverseRemapping().erase(iter);
							break;
						}
					}
					nextState.getConnectionPointReverseRemapping().emplace(
						connectionPoint,
						EvalConnectionPoint(subcircuitsPair.first, connectionEndId)
					);
					const EvalGate* evalGate = currentState.getGate(subcircuitsPair.first);
					auto connectionsIter = evalGate->connections.find(connectionEndId);
					if (connectionsIter != evalGate->connections.end()) {
						for (EvalConnectionPoint otherConnectionPoint : connectionsIter->second) {
							auto subcircuitDataAIter = subcircuits.find(otherConnectionPoint.gateId);
							if (subcircuitDataAIter != subcircuits.end()) {
								const Circuit* circuit = circuitManager.getCircuit(subcircuitDataAIter->second.circuitId).get();
								assert(circuit);
								const EvaluatorInternal& evaluatorInternalval = circuit->getEvaluator().getEvaluatorInternal();
								auto internalPointMappingIter = evaluatorInternalval.getPortToInternalPointMapping().find(otherConnectionPoint.connectionEndId);
								assert(internalPointMappingIter != evaluatorInternalval.getPortToInternalPointMapping().end());
								if (!internalPointMappingIter->second.connectionPoint.has_value()) continue;
								EvalConnectionPoint internalBottomPoint = evaluatorInternalval.mapFromTopConnectionPointToBottomConnectionPoint(
									internalPointMappingIter->second.connectionPoint.value()
								);
								auto otherSimulatorToThisSimulatorIdMappingIter = subcircuitDataAIter->second.otherSimulatorToThisSimulatorIdMapping.find(
									internalBottomPoint.gateId
								);
								assert(otherSimulatorToThisSimulatorIdMappingIter != subcircuitDataAIter->second.otherSimulatorToThisSimulatorIdMapping.end());
								otherConnectionPoint = EvalConnectionPoint(
									otherSimulatorToThisSimulatorIdMappingIter->second,
									internalBottomPoint.connectionEndId
								);
							}
							nextState.removeConnection(EvalConnection(
								emplaceResult.first->second,
								otherConnectionPoint
							));
							nextState.addConnection(EvalConnection(
								connectionPoint,
								otherConnectionPoint
							));
						}
					}
					emplaceResult.first->second = connectionPoint;
				}
			}
		}
	}
	evaluator.endEdit();
}
