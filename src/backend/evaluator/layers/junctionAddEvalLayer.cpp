#include "junctionAddEvalLayer.h"
#include "evalLayerState.h"

bool isConnectionEndIdSinglePin(EvalGateType gateType, connection_end_id_t connectionEndId) {
	// ignore lights, junctions, busses, custom blocks
	switch (getBlockType(gateType)) {
	case BlockType::SWITCH:
	case BlockType::BUTTON:
	case BlockType::TICK_BUTTON:
	case BlockType::BUFFER:
	case BlockType::NOT:
	case BlockType::CONSTANT_OFF:
	case BlockType::CONSTANT_ON:
	case BlockType::CONSTANT_X:
	case BlockType::CONSTANT_Z:
	case BlockType::TRISTATE_BUFFER:
		return true;
	case BlockType::AND:
	case BlockType::OR:
	case BlockType::XOR:
	case BlockType::NAND:
	case BlockType::NOR:
	case BlockType::XNOR:
		return connectionEndId == 1;
	default: return false;
	}
}

void JunctionAddEvalLayer::run(const EvalLayerState& currentState, EvalLayerState& nextState) {
	for (auto iter = currentState.getRemovedConnectionsBegin(); iter != currentState.getRemovedConnectionsEnd(); ++iter) {
		EvalConnection connection = *iter;
		const EvalGate* gateA = nextState.getGate(connection.connectionPointA.gateId);
		assert(gateA);
		const EvalGate* gateB = nextState.getGate(connection.connectionPointB.gateId);
		assert(gateB);
		eval_gate_id junctionAToRemove = 0;
		eval_gate_id junctionBToRemove = 0;
		if (isConnectionEndIdSinglePin(gateA->type, connection.connectionPointA.connectionEndId)) {
			auto connectionsIter = gateA->connections.find(connection.connectionPointA.connectionEndId);
			assert(connectionsIter != gateA->connections.end());
			eval_gate_id junctionId = connectionsIter->second.begin()->gateId;
			const EvalGate* junction = nextState.getGate(junctionId);
			if (junction->connections.at(0).size() == 2) {
				nextState.removeConnection(EvalConnection(connection.connectionPointA, EvalConnectionPoint(junctionId, 0)));
				junctionAToRemove = junctionId;
			}
			connection.connectionPointA = EvalConnectionPoint(junctionId, 0);
		}
		if (isConnectionEndIdSinglePin(gateB->type, connection.connectionPointA.connectionEndId)) {
			auto connectionsIter = gateB->connections.find(connection.connectionPointB.connectionEndId);
			assert(connectionsIter != gateB->connections.end());
			eval_gate_id junctionId = connectionsIter->second.begin()->gateId;
			const EvalGate* junction = nextState.getGate(junctionId);
			if (junction->connections.at(0).size() == 2) {
				nextState.removeConnection(EvalConnection(connection.connectionPointB, EvalConnectionPoint(junctionId, 0)));
				junctionBToRemove = junctionId;
			}
			connection.connectionPointB = EvalConnectionPoint(junctionId, 0);
		}
		nextState.removeConnection(connection);
		if (junctionAToRemove != 0) nextState.removeGate(junctionAToRemove);
		if (junctionBToRemove != 0) nextState.removeGate(junctionBToRemove);
	}
	for (auto iter = currentState.getRemovedGatesBegin(); iter != currentState.getRemovedGatesEnd(); ++iter) {
		nextState.passRemoveGate(*iter);
	}
	for (auto iter = currentState.getAddedGatesBegin(); iter != currentState.getAddedGatesEnd(); ++iter) {
		nextState.passAddGate(*iter);
	}
	for (auto iter = currentState.getAddedConnectionsBegin(); iter != currentState.getAddedConnectionsEnd(); ++iter) {
		EvalConnection connection = *iter;
		const EvalGate* gateA = nextState.getGate(connection.connectionPointA.gateId);
		assert(gateA);
		const EvalGate* gateB = nextState.getGate(connection.connectionPointB.gateId);
		assert(gateB);
		if (isConnectionEndIdSinglePin(gateA->type, connection.connectionPointA.connectionEndId)) {
			auto connectionsIter = gateA->connections.find(connection.connectionPointA.connectionEndId);
			if (connectionsIter == gateA->connections.end()) {
				eval_gate_id junctionId = nextState.getUnsedEvalGateId();
				nextState.addGate(junctionId, getEvalGateType(BlockType::JUNCTION));
				nextState.addConnection(EvalConnection(connection.connectionPointA, EvalConnectionPoint(junctionId, 0)));
				connection.connectionPointA = EvalConnectionPoint(junctionId, 0);
			} else {
				assert(connectionsIter->second.size() == 1);
				assert(isJunctionType(nextState.getGate(connectionsIter->second.begin()->gateId)->type));
				connection.connectionPointA = *(connectionsIter->second.begin());
			}
		}
		if (isConnectionEndIdSinglePin(gateB->type, connection.connectionPointA.connectionEndId)) {
			auto connectionsIter = gateB->connections.find(connection.connectionPointB.connectionEndId);
			if (connectionsIter == gateB->connections.end()) {
				eval_gate_id junctionId = nextState.getUnsedEvalGateId();
				nextState.addGate(junctionId, getEvalGateType(BlockType::JUNCTION));
				nextState.addConnection(EvalConnection(connection.connectionPointB, EvalConnectionPoint(junctionId, 0)));
				connection.connectionPointB = EvalConnectionPoint(junctionId, 0);
			} else {
				assert(connectionsIter->second.size() == 1);
				assert(isJunctionType(nextState.getGate(connectionsIter->second.begin()->gateId)->type));
				connection.connectionPointB = *(connectionsIter->second.begin());
			}
		}
		// if (connection.connectionPointA == connection.connectionPointB) continue; // wont happen
		nextState.addConnection(connection);
	}
}
