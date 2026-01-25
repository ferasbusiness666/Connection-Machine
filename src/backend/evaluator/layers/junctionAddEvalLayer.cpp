#include "junctionAddEvalLayer.h"
#include "evalLayerState.h"

BlockData::ConnectionData::PortType getConnectionEndIdSinglePinPortType(EvalGateType gateType, connection_end_id_t connectionEndId) {
	// ignore lights, junctions, busses, custom blocks
	switch (getBlockType(gateType)) {
	case BlockType::SWITCH:
	case BlockType::BUTTON:
	case BlockType::TICK_BUTTON:
	case BlockType::CONSTANT_OFF:
	case BlockType::CONSTANT_ON:
	case BlockType::CONSTANT_X:
	case BlockType::CONSTANT_Z:
		return BlockData::ConnectionData::PortType::OUTPUT;
	case BlockType::BUFFER:
	case BlockType::NOT:
		return connectionEndId == 0 ? BlockData::ConnectionData::PortType::INPUT : BlockData::ConnectionData::PortType::OUTPUT;
	case BlockType::TRISTATE_BUFFER:
		return connectionEndId == 2 ? BlockData::ConnectionData::PortType::OUTPUT : BlockData::ConnectionData::PortType::INPUT;;
	case BlockType::AND:
	case BlockType::OR:
	case BlockType::XOR:
	case BlockType::NAND:
	case BlockType::NOR:
	case BlockType::XNOR:
		return connectionEndId == 1 ? BlockData::ConnectionData::PortType::OUTPUT : BlockData::ConnectionData::PortType::NONE;;
	default:
		return BlockData::ConnectionData::PortType::NONE;
	}
}

void JunctionAddEvalLayer::run() {
	for (auto iter : currentState.getRemovedConnections()) {
		EvalConnection connection = iter.first;
		const EvalGate* gateA = nextState.getGate(connection.connectionPointA.gateId);
		assert(gateA);
		BlockData::ConnectionData::PortType portAType = getConnectionEndIdSinglePinPortType(gateA->type, connection.connectionPointA.connectionEndId);
		auto connectionsIterA = gateA->connections.find(connection.connectionPointA.connectionEndId);
		assert(connectionsIterA != gateA->connections.end());
		if (portAType == BlockData::ConnectionData::PortType::INPUT) {
			// should have only one connection to this connection point
			assert(connectionsIterA->second.size() == 1);
			EvalConnectionPoint otherConnectionPoint = *connectionsIterA->second.begin();
			if (otherConnectionPoint != connection.connectionPointB) {
				const EvalGate* junctionGate = nextState.getGate(otherConnectionPoint.gateId);
				assert(isJunctionType(junctionGate->type));
				nextState.removeConnection(EvalConnection(connection.connectionPointA, otherConnectionPoint), iter.second);
				connection.connectionPointA = otherConnectionPoint;
			}
		} else if (portAType == BlockData::ConnectionData::PortType::OUTPUT) {
			if (connectionsIterA->second.size() == 1) {
				EvalConnectionPoint otherConnectionPoint = *gateA->connections.at(connection.connectionPointA.connectionEndId).begin();
				if (otherConnectionPoint != connection.connectionPointB) {
					const EvalGate* junctionGate = nextState.getGate(otherConnectionPoint.gateId);
					assert(isJunctionType(junctionGate->type));
					nextState.removeConnection(EvalConnection(connection.connectionPointA, otherConnectionPoint), iter.second);
					connection.connectionPointA = otherConnectionPoint;
				}
			}
		}
		const EvalGate* gateB = nextState.getGate(connection.connectionPointB.gateId);
		assert(gateB);
		BlockData::ConnectionData::PortType portBType = getConnectionEndIdSinglePinPortType(gateB->type, connection.connectionPointB.connectionEndId);
		auto connectionsIterB = gateB->connections.find(connection.connectionPointB.connectionEndId);
		assert(connectionsIterB != gateB->connections.end());
		if (portBType == BlockData::ConnectionData::PortType::INPUT) {
			// should have only one connection to this connection point
			assert(connectionsIterB->second.size() == 1);
			EvalConnectionPoint otherConnectionPoint = *connectionsIterB->second.begin();
			if (otherConnectionPoint != connection.connectionPointA) {
				const EvalGate* junctionGate = nextState.getGate(otherConnectionPoint.gateId);
				assert(isJunctionType(junctionGate->type));
				nextState.removeConnection(EvalConnection(connection.connectionPointB, otherConnectionPoint), iter.second);
				connection.connectionPointB = otherConnectionPoint;
			}
		} else if (portBType == BlockData::ConnectionData::PortType::OUTPUT) {
			if (connectionsIterB->second.size() == 1) {
				EvalConnectionPoint otherConnectionPoint = *gateB->connections.at(connection.connectionPointB.connectionEndId).begin();
				if (otherConnectionPoint != connection.connectionPointA) {
					const EvalGate* junctionGate = nextState.getGate(otherConnectionPoint.gateId);
					assert(isJunctionType(junctionGate->type));
					nextState.removeConnection(EvalConnection(connection.connectionPointB, otherConnectionPoint), iter.second);
					connection.connectionPointB = otherConnectionPoint;
				}
			}
		}
		if (connection.connectionPointA == connection.connectionPointB) continue;
		nextState.removeConnection(connection, iter.second);
		if (portAType == BlockData::ConnectionData::PortType::INPUT) {
			if (!currentState.getRemovedGates().contains(connection.connectionPointB.gateId)) {
				if (currentState.getGate(connection.connectionPointB.gateId) == nullptr) {
					const EvalGate* gate = nextState.getGate(connection.connectionPointB.gateId);
					if (gate->connections.empty()) {
						nextState.removeGate(connection.connectionPointB.gateId);
					}
				}
			}
		} else if (portBType == BlockData::ConnectionData::PortType::INPUT) {
			if (!currentState.getRemovedGates().contains(connection.connectionPointA.gateId)) {
				if (currentState.getGate(connection.connectionPointA.gateId) == nullptr) {
					const EvalGate* gate = nextState.getGate(connection.connectionPointA.gateId);
					if (gate->connections.empty()) {
						nextState.removeGate(connection.connectionPointA.gateId);
					}
				}
			}
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
		BlockData::ConnectionData::PortType portAType = getConnectionEndIdSinglePinPortType(gateA->type, connection.connectionPointA.connectionEndId);
		if (portAType != BlockData::ConnectionData::PortType::NONE) {
			auto connectionsIter = gateA->connections.find(connection.connectionPointA.connectionEndId);
			if (connectionsIter != gateA->connections.end()) {
				bool foundJunction = false;
				for (EvalConnectionPoint connectionPoint : connectionsIter->second) {
					if (connectionPoint.connectionEndId == 0) {
						if (isJunctionType(nextState.getGate(connectionPoint.gateId)->type)) {
							foundJunction = true;
							nextState.addConnection(EvalConnection(connection.connectionPointA, connectionPoint), iter.second);
							connection.connectionPointA = connectionPoint;
							break;
						}
					}
				}
				if (!foundJunction && portAType == BlockData::ConnectionData::PortType::INPUT) { // this should only be the case if there is exactly one connection into this port
					// const EvalGate* curGateA = currentState.getGate(connection.connectionPointA.gateId);
					assert(connectionsIter->second.size() == 1);
					EvalConnectionPoint otherConnectionPoint = *connectionsIter->second.begin();
					EvalConnection otherConnection(connection.connectionPointA, otherConnectionPoint);
					if (otherConnectionPoint.connectionEndId == 0 && isJunctionType(currentState.getGate(otherConnectionPoint.gateId)->type)) {
						nextState.addConnection(otherConnection, iter.second);
						connection.connectionPointA = otherConnectionPoint;
					} else {
						eval_gate_id junctionId = nextState.getUnusedEvalGateId();
						nextState.addGate(junctionId, getEvalGateType(BlockType::JUNCTION));
						unsigned int otherConnectionWeight = nextState.getConnectionWeight(otherConnection);
						nextState.removeConnection(otherConnection, otherConnectionWeight);
						nextState.addConnection(EvalConnection(otherConnectionPoint, EvalConnectionPoint(junctionId, 0)), otherConnectionWeight);
						nextState.addConnection(EvalConnection(connection.connectionPointA, EvalConnectionPoint(junctionId, 0)), iter.second + otherConnectionWeight);
						connection.connectionPointA = EvalConnectionPoint(junctionId, 0);
					}
				}
			}
		}
		const EvalGate* gateB = nextState.getGate(connection.connectionPointB.gateId);
		assert(gateB);
		BlockData::ConnectionData::PortType portBType = getConnectionEndIdSinglePinPortType(gateB->type, connection.connectionPointB.connectionEndId);
		if (portBType != BlockData::ConnectionData::PortType::NONE) {
			assert(portBType != portAType);
			auto connectionsIter = gateB->connections.find(connection.connectionPointB.connectionEndId);
			if (connectionsIter != gateB->connections.end()) {
				bool foundJunction = false;
				for (EvalConnectionPoint connectionPoint : connectionsIter->second) {
					if (connectionPoint.connectionEndId == 0) {
						if (isJunctionType(nextState.getGate(connectionPoint.gateId)->type)) {
							foundJunction = true;
							nextState.addConnection(EvalConnection(connection.connectionPointB, connectionPoint), iter.second);
							connection.connectionPointB = connectionPoint;
							break;
						}
					}
				}
				if (!foundJunction && portBType == BlockData::ConnectionData::PortType::INPUT) { // this should only be the case if there is exactly one connection into this port
					// const EvalGate* curGateB = currentState.getGate(connection.connectionPointB.gateId);
					assert(connectionsIter->second.size() == 1);
					EvalConnectionPoint otherConnectionPoint = *connectionsIter->second.begin();
					EvalConnection otherConnection(connection.connectionPointB, otherConnectionPoint);
					if (otherConnectionPoint.connectionEndId == 0 && isJunctionType(currentState.getGate(otherConnectionPoint.gateId)->type)) {
						nextState.addConnection(otherConnection, iter.second);
						connection.connectionPointB = otherConnectionPoint;
					} else {
						eval_gate_id junctionId = nextState.getUnusedEvalGateId();
						nextState.addGate(junctionId, getEvalGateType(BlockType::JUNCTION));
						unsigned int otherConnectionWeight = nextState.getConnectionWeight(otherConnection);
						nextState.removeConnection(otherConnection, otherConnectionWeight);
						nextState.addConnection(EvalConnection(otherConnectionPoint, EvalConnectionPoint(junctionId, 0)), otherConnectionWeight);
						nextState.addConnection(EvalConnection(connection.connectionPointB, EvalConnectionPoint(junctionId, 0)), iter.second + otherConnectionWeight);
						connection.connectionPointB = EvalConnectionPoint(junctionId, 0);
					}
				}
			}
		}
		if (connection.connectionPointA == connection.connectionPointB) continue;
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
