#include "subcircuitEvalLayer.h"
#include "evalLayerState.h"
#include "backend/circuit/circuitManager.h"

void SubcircuitEvalLayer::run(const EvalLayerState& currentState, EvalLayerState& nextState) {
	for (auto iter = currentState.getRemovedConnectionsBegin(); iter != currentState.getRemovedConnectionsEnd(); ++iter) {
		nextState.removeConnection(iter->first, iter->second);
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
		auto subcircuitDataAIter = subcircuits.find(connection.connectionPointA.gateId);
		if (subcircuitDataAIter != subcircuits.end()) {
			const EvalGate* gate = currentState.getGate(connection.connectionPointA.gateId);
			assert(gate);
			const BlockData* blockData = circuitManager.getBlockDataManager().getBlockData(getBlockType(gate->type));
			assert(blockData);
			circuit_id_t circuitId = circuitManager.getCircuitBlockDataManager().getCircuitId(getBlockType(gate->type));
			const CircuitBlockData* circuitBlockData = circuitManager.getCircuitBlockDataManager().getCircuitBlockData(circuitId);
			assert(circuitBlockData);
		}
		auto subcircuitDataBIter = subcircuits.find(connection.connectionPointB.gateId);

		nextState.addConnection(connection, iter->second);
	}
}
