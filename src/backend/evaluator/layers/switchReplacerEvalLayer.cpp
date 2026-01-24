#include "switchReplacerEvalLayer.h"
#include "evalLayerState.h"

void SwitchReplacerEvalLayer::run() {
	for (auto iter : currentState.getRemovedConnections()) {
		EvalConnection connection = iter.first;
		if (connection.connectionPointA.connectionEndId == 1) {
			const EvalGate* gate = nextState.getGate(connection.connectionPointA.gateId);
			if (
				gate->type == getEvalGateType(BlockType::JUNCTION) ||
				gate->type == getEvalGateType(BlockType::SWITCH) || // test these incase we changed the block back already
				gate->type == getEvalGateType(BlockType::BUTTON) ||
				gate->type == getEvalGateType(BlockType::TICK_BUTTON)
			) {
				if (!currentState.getRemovedGates().contains(iter.first.connectionPointA.gateId)) {
					const EvalGate* curGate = currentState.getGate(connection.connectionPointA.gateId);
					if (!curGate->connections.contains(1)) {
						nextState.getConnectionPointRemapping().erase(connection.connectionPointA);
						nextState.getConnectionPointReverseRemapping().erase(EvalConnectionPoint(connection.connectionPointA.gateId, 0));
						nextState.changeGateType(connection.connectionPointA.gateId, curGate->type);
					}
				}
				connection.connectionPointA.connectionEndId = 0;
			}
		}
		if (connection.connectionPointB.connectionEndId == 1) {
			const EvalGate* gate = nextState.getGate(connection.connectionPointB.gateId);
			if (
				gate->type == getEvalGateType(BlockType::JUNCTION) ||
				gate->type == getEvalGateType(BlockType::SWITCH) || // test these incase we changed the block back already
				gate->type == getEvalGateType(BlockType::BUTTON) ||
				gate->type == getEvalGateType(BlockType::TICK_BUTTON)
			) {
				if (!currentState.getRemovedGates().contains(iter.first.connectionPointB.gateId)) {
					const EvalGate* curGate = currentState.getGate(connection.connectionPointB.gateId);
					if (!curGate->connections.contains(1)) {
						nextState.getConnectionPointRemapping().erase(connection.connectionPointB);
						nextState.getConnectionPointReverseRemapping().erase(EvalConnectionPoint(connection.connectionPointB.gateId, 0));
						nextState.changeGateType(connection.connectionPointB.gateId, curGate->type);
					}
				}
				connection.connectionPointB.connectionEndId = 0;
			}
		}
		nextState.removeConnection(connection, iter.second);
	}
	for (auto iter : currentState.getRemovedGates()) {
		nextState.getGateIdRemapping().erase(iter.first);
		nextState.getGateIdReverseRemapping().erase(iter.first);
		nextState.getConnectionPointRemapping().erase(EvalConnectionPoint(iter.first, 1));
		nextState.getConnectionPointReverseRemapping().erase(EvalConnectionPoint(iter.first, 0));
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
		if (connectionPoint.connectionEndId == 1 && nextState.getConnectionPointRemapping().contains(connectionPoint)) {
			connectionPoint.connectionEndId = 0;
		}
		nextState.addConnectionPointRemappingsUpdated(connectionPoint);
	}
}
