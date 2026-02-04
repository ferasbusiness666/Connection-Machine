#include "junctionAddEvalLayer.h"
#include "evalLayerState.h"
#include "backend/circuit/circuitManager.h"

#ifdef TRACY_PROFILER
#include <tracy/Tracy.hpp>
#endif

// before update
// URCL cpu
// addedGateCount:         6036
// removedGateCount:       0
// addedConnectionCount:   12227
// removedConnectionCount: 0
// 256 regs
// addedGateCount:         79386
// removedGateCount:       0
// addedConnectionCount:   121750
// removedConnectionCount: 0

// after update
// URCL cpu
// addedGateCount:         4025
// removedGateCount:       0
// addedConnectionCount:   10216
// removedConnectionCount: 0
// 256 regs
// addedGateCount:         50508
// removedGateCount:       0
// addedConnectionCount:   92872
// removedConnectionCount: 0

bool isConnectionEndIdInputSinglePin(EvalGateType gateType, connection_end_id_t connectionEndId, const CircuitManager& circuitManager) {
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
		return connectionEndId == 0;
	// case BlockType::AND:
	// case BlockType::OR:
	// case BlockType::XOR:
	// case BlockType::NAND:
	// case BlockType::NOR:
	// case BlockType::XNOR:
	// 	return connectionEndId == 1;
	default: {
		return false;
		// const BlockData* blockData = circuitManager.getBlockDataManager().getBlockData(getBlockType(gateType));
		// assert(blockData);
		// return blockData->isBus();
	}
	}
}

void JunctionAddEvalLayer::run() {
	#ifdef TRACY_PROFILER
	ZoneScopedN("JunctionAdd Run");
	#endif
	for (auto iter : currentState.getRemovedConnections()) {
		EvalConnection connection = iter.first;
		const EvalGate* gateA = nextState.getGate(connection.connectionPointA.gateId);
		assert(gateA);
		const EvalGate* gateB = nextState.getGate(connection.connectionPointB.gateId);
		assert(gateB);
		eval_gate_id junctionAToRemove = 0;
		eval_gate_id junctionBToRemove = 0;
		if (isConnectionEndIdInputSinglePin(gateA->type, connection.connectionPointA.connectionEndId, circuitManager)) {
			auto connectionsIter = gateA->connections.find(connection.connectionPointA.connectionEndId);
			assert(connectionsIter != gateA->connections.end());
			eval_gate_id junctionId = connectionsIter->second.begin()->gateId;
			const EvalGate* junction = nextState.getGate(junctionId);
			if (junction->connections.at(0).size() == 2) {
				nextState.removeConnection(EvalConnection(connection.connectionPointA, EvalConnectionPoint(junctionId, 0)), iter.second);
				junctionAToRemove = junctionId;
				if (currentState.getGate(connection.connectionPointA.gateId)) nextState.addGateIdRemappingsUpdated(connection.connectionPointA.gateId);
			}
			connection.connectionPointA = EvalConnectionPoint(junctionId, 0);
		}
		if (isConnectionEndIdInputSinglePin(gateB->type, connection.connectionPointB.connectionEndId, circuitManager)) {
			auto connectionsIter = gateB->connections.find(connection.connectionPointB.connectionEndId);
			assert(connectionsIter != gateB->connections.end());
			eval_gate_id junctionId = connectionsIter->second.begin()->gateId;
			const EvalGate* junction = nextState.getGate(junctionId);
			if (junction->connections.at(0).size() == 2) {
				nextState.removeConnection(EvalConnection(connection.connectionPointB, EvalConnectionPoint(junctionId, 0)), iter.second);
				junctionBToRemove = junctionId;
				if (currentState.getGate(connection.connectionPointB.gateId)) nextState.addGateIdRemappingsUpdated(connection.connectionPointB.gateId);
			}
			connection.connectionPointB = EvalConnectionPoint(junctionId, 0);
		}
		nextState.removeConnection(connection, iter.second);
		if (junctionAToRemove != 0) {
			nextState.removeGate(junctionAToRemove);
			nextState.releaseUnusedEvalGateId(junctionAToRemove);
		}
		if (junctionBToRemove != 0) {
			nextState.removeGate(junctionBToRemove);
			nextState.releaseUnusedEvalGateId(junctionAToRemove);
		}
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
		const EvalGate* gateA = nextState.getGate(connection.connectionPointA.gateId);
		assert(gateA);
		const EvalGate* gateB = nextState.getGate(connection.connectionPointB.gateId);
		assert(gateB);
		if (isConnectionEndIdInputSinglePin(gateA->type, connection.connectionPointA.connectionEndId, circuitManager)) {
			auto connectionsIter = gateA->connections.find(connection.connectionPointA.connectionEndId);
			if (connectionsIter == gateA->connections.end()) {
				eval_gate_id junctionId = nextState.getUnusedEvalGateId();
				nextState.addGate(junctionId, getEvalGateType(BlockType::JUNCTION));
				nextState.addConnection(EvalConnection(connection.connectionPointA, EvalConnectionPoint(junctionId, 0)), iter.second);
				nextState.addGateIdRemappingsUpdated(connection.connectionPointA.gateId);
				connection.connectionPointA = EvalConnectionPoint(junctionId, 0);
			} else {
				assert(connectionsIter->second.size() == 1);
				assert(isJunctionType(nextState.getGate(connectionsIter->second.begin()->gateId)->type));
				connection.connectionPointA = *(connectionsIter->second.begin());
			}
		}
		if (isConnectionEndIdInputSinglePin(gateB->type, connection.connectionPointB.connectionEndId, circuitManager)) {
			auto connectionsIter = gateB->connections.find(connection.connectionPointB.connectionEndId);
			if (connectionsIter == gateB->connections.end()) {
				eval_gate_id junctionId = nextState.getUnusedEvalGateId();
				nextState.addGate(junctionId, getEvalGateType(BlockType::JUNCTION));
				nextState.addConnection(EvalConnection(connection.connectionPointB, EvalConnectionPoint(junctionId, 0)), iter.second);
				nextState.addGateIdRemappingsUpdated(connection.connectionPointB.gateId);
				connection.connectionPointB = EvalConnectionPoint(junctionId, 0);
			} else {
				assert(connectionsIter->second.size() == 1);
				assert(isJunctionType(nextState.getGate(connectionsIter->second.begin()->gateId)->type));
				connection.connectionPointB = *(connectionsIter->second.begin());
			}
		}
		nextState.addConnection(connection, iter.second);
	}
	for (eval_gate_id gateId : currentState.getGateIdRemappingsUpdateds()) {
		assert(nextState.getGate(gateId));
		nextState.addGateIdRemappingsUpdated(gateId);
	}
	for (EvalConnectionPoint connectionPoint : currentState.getConnectionPointRemappingsUpdated()) {
		assert(nextState.getGate(connectionPoint.gateId));
		nextState.addConnectionPointRemappingsUpdated(connectionPoint);
	}
}
