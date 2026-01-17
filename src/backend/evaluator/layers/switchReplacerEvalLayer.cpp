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
				connection.connectionPointA.connectionEndId = 0;
				if (gate && !gate->connections.contains(1)) nextState.changeGateType(connection.connectionPointA.gateId, type);
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
				connection.connectionPointB.connectionEndId = 0;
				if (gate && !gate->connections.contains(1)) nextState.changeGateType(connection.connectionPointB.gateId, type);
			}
		}
		nextState.removeConnection(connection, iter.second);
	}
	for (auto iter : currentState.getRemovedGates()) {
		auto iterPair = nextState.getGateIdReverseRemapping().equal_range(iter.first);
		for (auto iter = iterPair.first; iter != iterPair.second; iter++) nextState.getGateIdRemapping().erase(iter->second);
		nextState.getGateIdReverseRemapping().erase(iterPair.first, iterPair.second);
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
				if (nextState.getGate(connection.connectionPointB.gateId)->type == gate->type) {;
					nextState.changeGateType(connection.connectionPointB.gateId, getEvalGateType(BlockType::JUNCTION));
				}
				connection.connectionPointB.connectionEndId = 0;
			}
		}
		nextState.addConnection(connection, iter.second);
	}
}
