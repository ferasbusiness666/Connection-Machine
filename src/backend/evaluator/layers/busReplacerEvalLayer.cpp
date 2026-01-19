#include "busReplacerEvalLayer.h"
#include "evalLayerState.h"

void BusReplacerEvalLayer::run() {
	for (auto iter : currentState.getRemovedConnections()) {
		nextState.removeConnection(iter.first, iter.second);
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
		nextState.addConnection(iter.first, iter.second);
	}
	for (eval_gate_id gateId : currentState.getGateIdRemappingsUpdateds()) {
		nextState.addGateIdRemappingsUpdated(gateId);
	}
	for (EvalConnectionPoint connectionPoint : currentState.getConnectionPointRemappingsUpdated()) {
		nextState.addConnectionPointRemappingsUpdated(connectionPoint);
	}
}
