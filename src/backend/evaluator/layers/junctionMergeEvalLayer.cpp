#include "junctionMergeEvalLayer.h"
#include "evalLayerState.h"

void JunctionMergeEvalLayer::run(const EvalLayerState& currentState, EvalLayerState& nextState) {
	std::set<eval_gate_id> junctionsToScan;
	for (auto iter = currentState.getRemovedConnectionsBegin(); iter != currentState.getRemovedConnectionsEnd(); ++iter) {
		EvalConnection connection = *iter;
		if (connection.connectionPointA.connectionEndId == 0) {
			auto remappingIter = nextState.getGateIdRemapping().find(connection.connectionPointA.gateId);
			if (remappingIter == nextState.getGateIdRemapping().end()) {
				logError("Failled to find junction gate remapping for gate id {}.", "JunctionMergeEvalLayer::run", connection.connectionPointA.gateId);
				continue;
			}
			connection.connectionPointA.gateId = remappingIter->second;
		}
		if (connection.connectionPointB.connectionEndId == 0) {
			auto remappingIter = nextState.getGateIdRemapping().find(connection.connectionPointB.gateId);
			if (remappingIter == nextState.getGateIdRemapping().end()) {
				logError("Failled to find junction gate remapping for gate id {}.", "JunctionMergeEvalLayer::run", connection.connectionPointB.gateId);
				continue;
			}
			connection.connectionPointB.gateId = remappingIter->second;
		}
		if (connection.connectionPointB == connection.connectionPointA) {
			const EvalGate* nextLayerJunction = nextState.getGate(connection.connectionPointA.gateId);
			assert(nextLayerJunction);
			assert(isJunctionType(nextLayerJunction->type));
			junctionsToScan.insert(connection.connectionPointA.gateId);
			junctionsToScan.insert(connection.connectionPointB.gateId);
		} else {
			nextState.removeConnection(connection);
		}
	}
	for (auto iter = currentState.getRemovedGatesBegin(); iter != currentState.getRemovedGatesEnd(); ++iter) {
		const EvalGate* gate = nextState.getGate(*iter);
		if (gate == nullptr) {
			auto remappingIter = nextState.getGateIdRemapping().find(*iter);
			if (remappingIter == nextState.getGateIdRemapping().end()) {
				logError("Failled to find junction gate remapping for gate id {}.", "JunctionMergeEvalLayer::run", *iter);
				continue;
			}
			if (currentState.getGate(remappingIter->second) == nullptr) continue;
			junctionsToScan.insert(remappingIter->second);
			auto iterPair = nextState.getGateIdReverseRemapping().equal_range(remappingIter->second);
			auto reverseRemappingIterToErase = iterPair.second;
			for (auto reverseRemappingIter = iterPair.first; reverseRemappingIter != iterPair.second; reverseRemappingIter++) {
				if (reverseRemappingIter->second == *iter) {
					reverseRemappingIterToErase = reverseRemappingIter;
				} else if (currentState.getGate(reverseRemappingIter->second)) {
					junctionsToScan.insert(reverseRemappingIter->second);
				}
			}
			assert(reverseRemappingIterToErase != iterPair.second);
			nextState.getGateIdReverseRemapping().erase(reverseRemappingIterToErase);
			nextState.getGateIdRemapping().erase(remappingIter);
			continue;
		}
		if (isJunctionType(gate->type)) {
			auto scanIter = junctionsToScan.find(*iter);
			if (scanIter != junctionsToScan.end()) {
				junctionsToScan.erase(scanIter);
				if (!gate->connections.empty()) {
					const std::unordered_set<EvalConnectionPoint>& connections = gate->connections.at(0);
					do {
						nextState.removeConnection(EvalConnection(EvalConnectionPoint(*iter, 0), *(connections.begin())));
					} while ((!gate->connections.empty()));
				}
			}
			auto iterPair = nextState.getGateIdReverseRemapping().equal_range(*iter);
			assert(iterPair.first != iterPair.second);
			for (auto reverseRemappingIter = iterPair.first; reverseRemappingIter != iterPair.second; reverseRemappingIter++) {
				if (reverseRemappingIter->second == *iter || currentState.getGate(reverseRemappingIter->second) == nullptr) continue;
				junctionsToScan.insert(reverseRemappingIter->second);
			}
		}
		nextState.passRemoveGate(*iter);
	}
	for (auto iter = currentState.getAddedGatesBegin(); iter != currentState.getAddedGatesEnd(); ++iter) {
		nextState.passAddGate(*iter);
	}
	for (auto iter = currentState.getAddedConnectionsBegin(); iter != currentState.getAddedConnectionsEnd(); ++iter) {
		EvalConnection connection = *iter;
		if (connection.connectionPointA.connectionEndId == 0) {
			auto remappingIter = nextState.getGateIdRemapping().find(connection.connectionPointA.gateId);
			if (remappingIter == nextState.getGateIdRemapping().end()) {
				logError("Failled to find junction gate remapping for gate id {}.", "JunctionMergeEvalLayer::run", connection.connectionPointA.gateId);
				continue;
			}
			connection.connectionPointA.gateId = remappingIter->second;
		}
		if (connection.connectionPointB.connectionEndId == 0) {
			auto remappingIter = nextState.getGateIdRemapping().find(connection.connectionPointB.gateId);
			if (remappingIter == nextState.getGateIdRemapping().end()) {
				logError("Failled to find junction gate remapping for gate id {}.", "JunctionMergeEvalLayer::run", connection.connectionPointB.gateId);
				continue;
			}
			connection.connectionPointB.gateId = remappingIter->second;
		}
		if (connection.connectionPointB == connection.connectionPointA) continue;
		const EvalGate* gateA = nextState.getGate(connection.connectionPointA.gateId);
		const EvalGate* gateB = nextState.getGate(connection.connectionPointB.gateId);
		if (isJunctionType(gateA->type) && isJunctionType(gateB->type)) {
			assert(currentState.getGate(connection.connectionPointA.gateId));
			junctionsToScan.insert(connection.connectionPointA.gateId);
		} else {
			nextState.addConnection(connection);
			// logInfo("added connetion {}, {}, {}, {}", "", evalConnection.connectionPointA.gateId, evalConnection.connectionPointA.connectionEndId, evalConnection.connectionPointB.gateId, evalConnection.connectionPointB.connectionEndId);
		}
	}
	// logInfo(junctionsToScan.size());
	while (!junctionsToScan.empty()) {
		eval_gate_id gateId = *junctionsToScan.begin();
		junctionsToScan.erase(junctionsToScan.begin());
		auto [junctions, otherConnectionPoints, gateType] = gatherJunctionGroup(gateId, currentState);
		// logInfo("Merging group of {} gates. Final ID: {}", "", junctions.size(), gateId);
		bool foundPullDown = false;
		bool foundPullUp = false;
		for (eval_gate_id junctionId : junctions) {
			junctionsToScan.erase(junctionId);
			const EvalGate* junction = nextState.getGate(junctionId);
			if (junction) {
				if (!junction->connections.empty()) {
					const std::unordered_set<EvalConnectionPoint>& connections = junction->connections.at(0);
					do {
						// logInfo("removed connetion {}, {}, {}, {}", "", junctionId, 0, connections.begin()->gateId, connections.begin()->connectionEndId);
						nextState.removeConnection(EvalConnection(EvalConnectionPoint(junctionId, 0), *(connections.begin())));
					} while ((!junction->connections.empty()));
				}
			}
			auto pair = nextState.getGateIdRemapping().emplace(junctionId, gateId);
			if (!pair.second) {
				auto iterPair = nextState.getGateIdReverseRemapping().equal_range(pair.first->second);
				for (auto iter = iterPair.first; iter != iterPair.second; iter++) {
					if (iter->second == junctionId) {
						nextState.getGateIdReverseRemapping().erase(iter);
						break;
					}
				}
				pair.first->second = gateId;
			}
			nextState.getGateIdReverseRemapping().emplace(gateId, junctionId);
			if (junction) nextState.removeGate(junctionId);
		}

		nextState.addGate(gateId, gateType);
		for (EvalConnectionPoint connectionPoint : otherConnectionPoints) {
			nextState.addConnection(EvalConnection(EvalConnectionPoint(gateId, 0), connectionPoint));
		}
	}
}

std::tuple<std::vector<eval_gate_id>, std::unordered_set<EvalConnectionPoint>, EvalGateType> JunctionMergeEvalLayer::gatherJunctionGroup(eval_gate_id gateId, const EvalLayerState& evalLayerState) const {
	std::vector<eval_gate_id> junctions = { gateId };
	std::unordered_set<EvalConnectionPoint> visted = { EvalConnectionPoint(gateId, 0) };
	bool foundPullDown = false;
	bool foundPullUp = false;
	for (unsigned int i = 0; i < junctions.size(); i++) {
		const EvalGate* evalGate = evalLayerState.getGate(junctions[i]);
		if (isJunctionType(evalGate->type)) {
			if (evalGate->type == getEvalGateType(BlockType::JUNCTION_L)) foundPullDown = true;
			if (evalGate->type == getEvalGateType(BlockType::JUNCTION_H)) foundPullUp = true;
			if (evalGate->type == getEvalGateType(BlockType::JUNCTION_X)) foundPullDown = foundPullUp = true;
			if (!evalGate->connections.empty()) {
				for (EvalConnectionPoint connection : evalGate->connections.at(0)) {
					if (connection.connectionEndId != 0) {
						visted.insert(connection);
						continue;
					}
					if (!visted.insert(connection).second) continue;
					junctions.push_back(connection.gateId);
				}
			}
		} else {
			if (i + 1 == junctions.size()) {
				junctions.pop_back();
			} else {
				junctions[i] = junctions.back();
				junctions.pop_back();
				i--;
			}
		}
	}
	for (eval_gate_id junction : junctions) {
		visted.erase(EvalConnectionPoint(junction, 0));
		const EvalGate* evalGate = evalLayerState.getGate(junction);
		assert(isJunctionType(evalGate->type));
	}
	EvalGateType gateType = getEvalGateType(BlockType::JUNCTION);
	if (foundPullDown) {
		if (foundPullUp) gateType = getEvalGateType(BlockType::JUNCTION_X);
		else gateType = getEvalGateType(BlockType::JUNCTION_L);
	} else if (foundPullUp) {
		gateType = getEvalGateType(BlockType::JUNCTION_H);
	}
	return { junctions, visted, gateType };
}
