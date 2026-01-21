#include "switchReplacerEvalLayer.h"
#include "evalLayerState.h"

void SwitchReplacerEvalLayer::run() {
	for (auto iter : currentState.getRemovedConnections()) {
		EvalConnection connection = iter.first;
		if (connection.connectionPointA.connectionEndId == 1) {
			const EvalGate* gate = currentState.getGate(connection.connectionPointA.gateId);
			EvalGateType type = gate ? gate->type : currentState.getRemovedGates().at(iter.first.connectionPointA.gateId);
			if (
				type == getEvalGateType(BlockType::SWITCH) ||
				type == getEvalGateType(BlockType::BUTTON) ||
				type == getEvalGateType(BlockType::TICK_BUTTON)
			) {
				if (gate && !gate->connections.contains(1)) {
					nextState.getConnectionPointRemapping().erase(connection.connectionPointA);
					auto iterPair = nextState.getConnectionPointReverseRemapping().equal_range(EvalConnectionPoint(connection.connectionPointA.gateId, 0));
					for (auto reverseRemappingIter = iterPair.first; reverseRemappingIter != iterPair.second; reverseRemappingIter++) {
						if (reverseRemappingIter->second == connection.connectionPointA) {
							nextState.getConnectionPointReverseRemapping().erase(reverseRemappingIter);
						}
						break;
					}
					nextState.changeGateType(connection.connectionPointA.gateId, type);
				}
				connection.connectionPointA.connectionEndId = 0;
			}
		}
		if (connection.connectionPointB.connectionEndId == 1) {
			const EvalGate* gate = currentState.getGate(connection.connectionPointB.gateId);
			EvalGateType type = gate ? gate->type : currentState.getRemovedGates().at(iter.first.connectionPointB.gateId);
			if (
				type == getEvalGateType(BlockType::SWITCH) ||
				type == getEvalGateType(BlockType::BUTTON) ||
				type == getEvalGateType(BlockType::TICK_BUTTON)
			) {
				if (gate && !gate->connections.contains(1)) {
					nextState.getConnectionPointRemapping().erase(connection.connectionPointB);
					auto iterPair = nextState.getConnectionPointReverseRemapping().equal_range(EvalConnectionPoint(connection.connectionPointB.gateId, 0));
					for (auto reverseRemappingIter = iterPair.first; reverseRemappingIter != iterPair.second; reverseRemappingIter++) {
						if (reverseRemappingIter->second == connection.connectionPointB) {
							nextState.getConnectionPointReverseRemapping().erase(reverseRemappingIter);
						}
						break;
					}
					nextState.changeGateType(connection.connectionPointB.gateId, type);
				}
				connection.connectionPointB.connectionEndId = 0;
			}
		}
		nextState.removeConnection(connection, iter.second);
	}
	for (auto iter : currentState.getRemovedGates()) {
		nextState.getGateIdRemapping().erase(iter.first);
		nextState.getGateIdReverseRemapping().erase(iter.first);
		nextState.removeGate(iter.first);
	}
	for (auto iter : currentState.getAddedGates()) {
		nextState.getGateIdRemapping().emplace(iter.first, iter.first);
		nextState.getGateIdReverseRemapping().emplace(iter.first, iter.first);
		nextState.addGate(iter.first, iter.second);
	}
	for (auto iter : currentState.getAddedConnections()) {
		EvalConnection connection = iter.first;
		if (connection.connectionPointA.connectionEndId == 1) {
			const EvalGate* gate = currentState.getGate(connection.connectionPointA.gateId);
			if (
				gate->type == getEvalGateType(BlockType::SWITCH) ||
				gate->type == getEvalGateType(BlockType::BUTTON) ||
				gate->type == getEvalGateType(BlockType::TICK_BUTTON)
			) {
				if (nextState.getGate(connection.connectionPointA.gateId)->type == gate->type) {
					nextState.getConnectionPointRemapping().emplace(connection.connectionPointA, EvalConnectionPoint(connection.connectionPointA.gateId, 0));
					nextState.getConnectionPointReverseRemapping().emplace(EvalConnectionPoint(connection.connectionPointA.gateId, 0), connection.connectionPointA);
					nextState.addConnectionPointRemappingsUpdated(EvalConnectionPoint(connection.connectionPointA.gateId, 0));
					nextState.changeGateType(connection.connectionPointA.gateId, getEvalGateType(BlockType::JUNCTION));
				}
				connection.connectionPointA.connectionEndId = 0;
			}
		}
		if (connection.connectionPointB.connectionEndId == 1) {
			const EvalGate* gate = currentState.getGate(connection.connectionPointB.gateId);
			if (
				gate->type == getEvalGateType(BlockType::SWITCH) ||
				gate->type == getEvalGateType(BlockType::BUTTON) ||
				gate->type == getEvalGateType(BlockType::TICK_BUTTON)
			) {
				if (nextState.getGate(connection.connectionPointB.gateId)->type == gate->type) {
					nextState.getConnectionPointRemapping().emplace(connection.connectionPointB, EvalConnectionPoint(connection.connectionPointB.gateId, 0));
					nextState.getConnectionPointReverseRemapping().emplace(EvalConnectionPoint(connection.connectionPointB.gateId, 0), connection.connectionPointB);
					nextState.addConnectionPointRemappingsUpdated(EvalConnectionPoint(connection.connectionPointB.gateId, 0));
					nextState.changeGateType(connection.connectionPointB.gateId, getEvalGateType(BlockType::JUNCTION));
				}
				connection.connectionPointB.connectionEndId = 0;
			}
		}
		nextState.addConnection(connection, iter.second);
	}
	for (eval_gate_id gateId : currentState.getGateIdRemappingsUpdateds()) {
		nextState.addGateIdRemappingsUpdated(gateId);
	}
	for (EvalConnectionPoint connectionPoint : currentState.getConnectionPointRemappingsUpdated()) {
		if (connectionPoint.connectionEndId == 1) {
			const EvalGate* gate = nextState.getGate(connectionPoint.gateId);
			assert(gate);
			if (isJunctionType(gate->type)) {
				connectionPoint.connectionEndId = 0;
			}
		}
		nextState.addConnectionPointRemappingsUpdated(connectionPoint);
	}
}
