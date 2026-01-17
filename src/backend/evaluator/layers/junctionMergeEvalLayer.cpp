#include "junctionMergeEvalLayer.h"
#include "evalLayerState.h"

void JunctionMergeEvalLayer::run() {
	std::set<eval_gate_id> junctionsToScan;
	for (auto iter : currentState.getRemovedConnections()) {
		EvalConnection connection = iter.first;
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
			if (currentState.getGate(iter.first.connectionPointA.gateId)) junctionsToScan.insert(iter.first.connectionPointA.gateId);
			if (currentState.getGate(iter.first.connectionPointB.gateId)) junctionsToScan.insert(iter.first.connectionPointB.gateId);
		} else {
			nextState.removeConnection(connection, iter.second);
		}
	}
	for (auto iter : currentState.getRemovedGates()) {
		const EvalGate* gate = nextState.getGate(iter.first);
		if (gate == nullptr) {
			auto remappingIter = nextState.getGateIdRemapping().find(iter.first);
			if (remappingIter == nextState.getGateIdRemapping().end()) {
				continue;
			}
			if (currentState.getGate(remappingIter->second) == nullptr) continue;
			auto iterPair = nextState.getGateIdReverseRemapping().equal_range(remappingIter->second);
			auto reverseRemappingIterToErase = iterPair.second;
			for (auto reverseRemappingIter = iterPair.first; reverseRemappingIter != iterPair.second; reverseRemappingIter++) {
				if (reverseRemappingIter->second == iter.first) {
					reverseRemappingIterToErase = reverseRemappingIter;
				} else if (currentState.getGate(reverseRemappingIter->second)) {
					assert(currentState.getGate(reverseRemappingIter->second));
					junctionsToScan.insert(reverseRemappingIter->second);
				}
			}
			assert(reverseRemappingIterToErase != iterPair.second);
			nextState.getGateIdReverseRemapping().erase(reverseRemappingIterToErase);
			nextState.getGateIdRemapping().erase(remappingIter);
			continue;
		}
		if (isJunctionType(iter.second)) {
			auto scanIter = junctionsToScan.find(iter.first);
			if (scanIter != junctionsToScan.end()) {
				junctionsToScan.erase(scanIter);
				if (!gate->connections.empty()) {
					const std::unordered_set<EvalConnectionPoint>& connections = gate->connections.at(0);
					do {
						nextState.removeConnection(EvalConnection(EvalConnectionPoint(iter.first, 0), *(connections.begin())));
					} while ((!gate->connections.empty()));
				}
			}
			auto iterPair = nextState.getGateIdReverseRemapping().equal_range(iter.first);
			assert(iterPair.first != iterPair.second);
			for (auto reverseRemappingIter = iterPair.first; reverseRemappingIter != iterPair.second; reverseRemappingIter++) {
				if (reverseRemappingIter->second == iter.first || currentState.getGate(reverseRemappingIter->second) == nullptr) continue;
				junctionsToScan.insert(reverseRemappingIter->second);
			}
		}
		auto iterPair = nextState.getGateIdReverseRemapping().equal_range(iter.first);
		for (auto iter = iterPair.first; iter != iterPair.second; iter++) nextState.getGateIdRemapping().erase(iter->second);
		nextState.getGateIdReverseRemapping().erase(iterPair.first, iterPair.second);
		nextState.removeGate(iter.first);
	}
	for (auto iter : currentState.getAddedGates()) {
		junctionsToScan.erase(iter.first);
		nextState.getGateIdRemapping().emplace(iter.first, iter.first);
		nextState.getGateIdReverseRemapping().emplace(iter.first, iter.first);
		nextState.addGate(iter.first, iter.second);
	}
	for (auto iter : currentState.getAddedConnections()) {
		EvalConnection connection = iter.first;
		if (connection.connectionPointA.connectionEndId == 0) {
			auto remappingIter = nextState.getGateIdRemapping().find(connection.connectionPointA.gateId);
			if (remappingIter == nextState.getGateIdRemapping().end()) {
				if (junctionsToScan.contains(connection.connectionPointA.gateId)) continue;
				logError("Failled to find junction gate remapping for gate id {}.", "JunctionMergeEvalLayer::run", connection.connectionPointA.gateId);
				continue;
			}
			connection.connectionPointA.gateId = remappingIter->second;
		}
		if (connection.connectionPointB.connectionEndId == 0) {
			auto remappingIter = nextState.getGateIdRemapping().find(connection.connectionPointB.gateId);
			if (remappingIter == nextState.getGateIdRemapping().end()) {
				if (junctionsToScan.contains(connection.connectionPointB.gateId)) continue;
				logError("Failled to find junction gate remapping for gate id {}.", "JunctionMergeEvalLayer::run", connection.connectionPointB.gateId);
				continue;
			}
			connection.connectionPointB.gateId = remappingIter->second;
		}
		if (connection.connectionPointB == connection.connectionPointA) continue;
		const EvalGate* gateA = nextState.getGate(connection.connectionPointA.gateId);
		const EvalGate* gateB = nextState.getGate(connection.connectionPointB.gateId);
		if (isJunctionType(gateA->type) && isJunctionType(gateB->type)) {
			assert(currentState.getGate(iter.first.connectionPointA.gateId));
			junctionsToScan.insert(iter.first.connectionPointA.gateId);
		} else {
			nextState.addConnection(connection);
		}
	}
	std::set<eval_gate_id> gatesTokeep;
	while (!junctionsToScan.empty()) {
		eval_gate_id gateId = *junctionsToScan.begin();
		junctionsToScan.erase(junctionsToScan.begin());
		auto [junctions, otherConnectionPoints, gateType] = gatherJunctionGroup(gateId, currentState);
		EvalGateType oldType = currentState.getGate(gateId)->type;
		bool foundPullDown = false;
		bool foundPullUp = false;
		for (eval_gate_id junctionId : junctions) {
			junctionsToScan.erase(junctionId);
			auto pair = nextState.getGateIdRemapping().emplace(junctionId, gateId);
			if (!pair.second) {
				if (junctionId != pair.first->second && (!gatesTokeep.contains(pair.first->second)) && nextState.getGate(pair.first->second)) {
					nextState.removeGate(pair.first->second);
				}
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
			if (nextState.getGate(junctionId)) nextState.removeGate(junctionId);
		}
		gatesTokeep.insert(gateId);
		nextState.addGate(gateId, gateType);
		for (std::pair<EvalConnectionPoint, unsigned int> pair : otherConnectionPoints) {
			nextState.addConnection(EvalConnection(EvalConnectionPoint(gateId, 0), pair.first), pair.second);
		}
	}
}

std::tuple<
	std::vector<eval_gate_id>,
	std::unordered_map<EvalConnectionPoint, unsigned int>,
	EvalGateType
> JunctionMergeEvalLayer::gatherJunctionGroup(eval_gate_id gateId, const EvalLayerState& evalLayerState) const {
	std::vector<eval_gate_id> junctions = { gateId };
	std::unordered_map<EvalConnectionPoint, unsigned int> visted = { {EvalConnectionPoint(gateId, 0), 1} };
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
					auto pair = visted.emplace(connection, 1);
					if (pair.second) {
						junctions.push_back(connection.gateId);
					} else {
						pair.first->second += 1;
					}
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
