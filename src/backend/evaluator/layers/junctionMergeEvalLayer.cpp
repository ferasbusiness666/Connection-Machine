#include "junctionMergeEvalLayer.h"
#include "evalLayerState.h"

bool isJunctionType(EvalGateType gateType) {
	return (
		gateType == getEvalGateType(BlockType::JUNCTION) || gateType == getEvalGateType(BlockType::JUNCTION_L) || gateType == getEvalGateType(BlockType::JUNCTION_H) ||
		gateType == getEvalGateType(BlockType::JUNCTION_X)
	);
}

void JunctionMergeEvalLayer::run(const EvalLayerState& currentState, EvalLayerState& nextState) {
	std::set<eval_gate_id> junctionsToScan;
	for (auto iter = currentState.getRemovedConnectionsBegin(); iter != currentState.getRemovedConnectionsEnd(); ++iter) {
		const EvalConnection& evalConnection = *iter;
		auto remappingIterA = nextState.getEvalGateIdRemapping().find(evalConnection.connectionPointA.gateId);
		if (remappingIterA == nextState.getEvalGateIdRemapping().end()) {
			logError("Failled to find junction gate remapping for gate id {}.", "JunctionMergeEvalLayer::run", evalConnection.connectionPointA.gateId);
			continue;
		}
		auto remappingIterB = nextState.getEvalGateIdRemapping().find(evalConnection.connectionPointB.gateId);
		if (remappingIterB == nextState.getEvalGateIdRemapping().end()) {
			logError("Failled to find junction gate remapping for gate id {}.", "JunctionMergeEvalLayer::run", evalConnection.connectionPointB.gateId);
			continue;
		}
		if (remappingIterA->second == remappingIterB->second) {
			const EvalGate* nextLayerJunction = nextState.getGate(remappingIterA->second);
			assert(nextLayerJunction);
			assert(isJunctionType(nextLayerJunction->type));
			junctionsToScan.insert(evalConnection.connectionPointB.gateId);
			junctionsToScan.insert(evalConnection.connectionPointB.gateId);
		} else {
			nextState.passRemoveConnection(*iter);
		}
	}
	for (auto iter = currentState.getRemovedGatesBegin(); iter != currentState.getRemovedGatesEnd(); ++iter) {
		auto scanIter = junctionsToScan.find(*iter);
		if (scanIter != junctionsToScan.end()) junctionsToScan.erase(scanIter);
		nextState.passRemoveGate(*iter); // good
	}
	for (auto iter = currentState.getAddedGatesBegin(); iter != currentState.getAddedGatesEnd(); ++iter) {
		nextState.passAddGate(*iter); // good
	}
	for (auto iter = currentState.getAddedConnectionsBegin(); iter != currentState.getAddedConnectionsEnd(); ++iter) {
		const EvalConnection& evalConnection = *iter;
		if (evalConnection.connectionPointA.connectionEndId == 0 && evalConnection.connectionPointB.connectionEndId == 0) {
			auto remappingIterA = nextState.getEvalGateIdRemapping().find(evalConnection.connectionPointA.gateId);
			if (remappingIterA == nextState.getEvalGateIdRemapping().end()) {
				logError("Failled to find junction gate remapping for gate id {}.", "JunctionMergeEvalLayer::run", evalConnection.connectionPointA.gateId);
				continue;
			}
			auto remappingIterB = nextState.getEvalGateIdRemapping().find(evalConnection.connectionPointB.gateId);
			if (remappingIterB == nextState.getEvalGateIdRemapping().end()) {
				logError("Failled to find junction gate remapping for gate id {}.", "JunctionMergeEvalLayer::run", evalConnection.connectionPointB.gateId);
				continue;
			}
			if (remappingIterA->second == remappingIterB->second) {
				const EvalGate* nextLayerJunction = nextState.getGate(remappingIterA->second);
				assert(nextLayerJunction);
				assert(isJunctionType(nextLayerJunction->type));
				continue;
			}
		}
		const EvalGate* gateA = nextState.getGate(evalConnection.connectionPointA.gateId);
		const EvalGate* gateB = nextState.getGate(evalConnection.connectionPointB.gateId);
		if (isJunctionType(gateA->type) && isJunctionType(gateB->type)) {
			junctionsToScan.insert(evalConnection.connectionPointA.gateId);
		} else {
			nextState.passAddConnection(evalConnection);
		}
	}
	while (!junctionsToScan.empty()) {
		eval_gate_id gateId = *junctionsToScan.begin();
		junctionsToScan.erase(junctionsToScan.begin());
		auto [junctions, otherConnectionPoints, gateType] = gatherJunctionGroup(gateId, currentState);
		bool foundPullDown = false;
		bool foundPullUp = false;
		for (eval_gate_id junctionId : junctions) {
			junctionsToScan.erase(junctionId);
			const EvalGate* junction = nextState.getGate(junctionId);
			if (!junction) continue;
			if (!junction->connections.empty()) {
				const std::unordered_set<EvalConnectionPoint>& connections = junction->connections.at(0);
				do {
					nextState.removeConnection(EvalConnection(EvalConnectionPoint(junctionId, 0), *(connections.begin())));
				} while ((!junction->connections.empty()));
			}
			auto pair = nextState.getEvalGateIdRemapping().emplace(junctionId, gateId);
			if (!pair.second) {
				auto iterPair = nextState.getEvalGateIdReverseRemapping().equal_range(pair.first->second);
				for (auto iter = iterPair.first; iter != iterPair.second; iter++) {
					if (iter->second == junctionId) {
						nextState.getEvalGateIdReverseRemapping().erase(iter);
						break;
					}
				}
				nextState.getEvalGateIdReverseRemapping().emplace(gateId, junctionId);
				pair.first->second = gateId;
			}
			nextState.removeGate(junctionId);
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
