#include "switchReplacerEvalLayer.h"
#include "evalLayerState.h"

void SwitchReplacerEvalLayer::run(const EvalLayerState& currentState, EvalLayerState& nextState) {
	for (auto iter = currentState.getRemovedConnectionsBegin(); iter != currentState.getRemovedConnectionsEnd(); ++iter) {
		EvalConnection connection = iter->first;
		if (connection.connectionPointA.connectionEndId == 1) {
			const EvalGate* gate = currentState.getGate(connection.connectionPointA.gateId);
			if (
				gate->type == getEvalGateType(BlockType::SWITCH) ||
				gate->type == getEvalGateType(BlockType::BUTTON) ||
				gate->type == getEvalGateType(BlockType::TICK_BUTTON)
			) {
				if (nextState.getGate(connection.connectionPointA.gateId)->type != gate->type) {

				}
			}
		}
		if (connection.connectionPointB.connectionEndId == 1) {
			const EvalGate* gate = currentState.getGate(connection.connectionPointB.gateId);
			if (
				gate->type == getEvalGateType(BlockType::SWITCH) ||
				gate->type == getEvalGateType(BlockType::BUTTON) ||
				gate->type == getEvalGateType(BlockType::TICK_BUTTON)
			) {
				if (nextState.getGate(connection.connectionPointB.gateId)->type != gate->type) {

				}
			}
		}
		nextState.removeConnection(connection, iter->second);
	}
	for (auto iter = currentState.getRemovedGatesBegin(); iter != currentState.getRemovedGatesEnd(); ++iter) {
		auto iterPair = nextState.getGateIdReverseRemapping().equal_range(*iter);
		for (auto iter = iterPair.first; iter != iterPair.second; iter++) nextState.getGateIdRemapping().erase(iter->second);
		nextState.getGateIdReverseRemapping().erase(iterPair.first, iterPair.second);
		nextState.removeGate(*iter);
	}
	for (auto iter = currentState.getAddedGatesBegin(); iter != currentState.getAddedGatesEnd(); ++iter) {
		const EvalGate* evalGate = currentState.getGate(*iter);
		nextState.getGateIdRemapping().emplace(*iter, *iter);
		nextState.getGateIdReverseRemapping().emplace(*iter, *iter);
		nextState.addGate(*iter, evalGate->type);
	}
	for (auto iter = currentState.getAddedConnectionsBegin(); iter != currentState.getAddedConnectionsEnd(); ++iter) {
		EvalConnection connection = iter->first;
		if (connection.connectionPointA.connectionEndId == 1) {
			const EvalGate* gate = currentState.getGate(connection.connectionPointA.gateId);
			if (
				gate->type == getEvalGateType(BlockType::SWITCH) ||
				gate->type == getEvalGateType(BlockType::BUTTON) ||
				gate->type == getEvalGateType(BlockType::TICK_BUTTON)
			) {
				if (nextState.getGate(connection.connectionPointA.gateId)->type == gate->type) {
					nextState.changeGateType(connection.connectionPointA.gateId, getEvalGateType(BlockType::JUNCTION));
					connection.connectionPointA.connectionEndId = 0;
				}
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
					connection.connectionPointB.connectionEndId = 0;
				}
			}
		}
		nextState.addConnection(connection, iter->second);
	}
}
