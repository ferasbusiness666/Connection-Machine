#include "busReplacerEvalLayer.h"
#include "backend/circuit/circuitManager.h"
#include "evalLayerState.h"

void BusReplacerEvalLayer::run() {
	for (auto iter : currentState.getRemovedConnections()) {
		EvalConnection connection = iter.first;
		auto remappingIterA = nextState.getConnectionPointRemapping().find(connection .connectionPointA);
		if (remappingIterA != nextState.getConnectionPointRemapping().end()) {
			connection.connectionPointA = remappingIterA->second;
		}
		auto remappingIterB = nextState.getConnectionPointRemapping().find(connection .connectionPointB);
		if (remappingIterB != nextState.getConnectionPointRemapping().end()) {
			connection.connectionPointB = remappingIterB->second;
		}
		nextState.removeConnection(connection, iter.second);
	}
	for (auto iter : currentState.getRemovedGates()) {
		auto bussesiter = busses.find(iter.first);
		if (bussesiter == busses.end()) {

		}

		nextState.getGateIdRemapping().erase(iter.first);
		nextState.getGateIdReverseRemapping().erase(iter.first);
		nextState.removeGate(iter.first);
	}
	for (auto iter : currentState.getAddedGates()) {
		const BlockData* blockData = circuitManager.getBlockDataManager().getBlockData(getBlockType(iter.second));
		assert(blockData);
		if (blockData->isBus()) {
			auto iterBool = busses.try_emplace(iter.first);
			for (std::pair<connection_end_id_t, BlockData::ConnectionData> connectionData : blockData->getConnectionsSafe()) {
				auto iterBool2 = iterBool.first->second.connectionEnds.try_emplace(connectionData.first);
				const std::variant<unsigned int, std::vector<unsigned int>>& bitConfiguration = connectionData.second.bitConfiguration;
				if (std::holds_alternative<unsigned int>(bitConfiguration)) {
					for (unsigned int lane = 0; lane < std::get<unsigned int>(bitConfiguration); lane++) {
						iterBool2.first->second.push_back(lane);
						auto pair = iterBool.first->second.junctions.emplace(lane, 0);
						if (pair.second) {
							eval_gate_id junctionId = nextState.getUnusedEvalGateId();
							pair.first->second = junctionId;
							nextState.addGate(junctionId, getEvalGateType(BlockType::JUNCTION));
						}
						nextState.getConnectionPointReverseRemapping().emplace(
							EvalConnectionPoint(pair.first->second, 0),
							EvalConnectionPoint(iter.first, connectionData.first)
						);
					}
				} else {
					for (unsigned int lane : std::get<std::vector<unsigned int>>(bitConfiguration)) {
						iterBool2.first->second.push_back(lane);
						auto pair = iterBool.first->second.junctions.emplace(lane, 0);
						if (pair.second) {
							eval_gate_id junctionId = nextState.getUnusedEvalGateId();
							pair.first->second = junctionId;
							nextState.addGate(junctionId, getEvalGateType(BlockType::JUNCTION));
						}
						nextState.getConnectionPointRemapping().emplace(
							EvalConnectionPoint(iter.first, connectionData.first),
							EvalConnectionPoint(pair.first->second, 0)
						);
						nextState.getConnectionPointReverseRemapping().emplace(
							EvalConnectionPoint(pair.first->second, 0),
							EvalConnectionPoint(iter.first, connectionData.first)
						);
					}
				}
			}
		} else {
			nextState.getGateIdRemapping().emplace(iter.first, iter.first);
			nextState.getGateIdReverseRemapping().emplace(iter.first, iter.first);
			nextState.addGate(iter.first, iter.second);
		}
	}
	for (auto iter : currentState.getAddedConnections()) {
		auto bussesIterA = busses.find(iter.first.connectionPointA.gateId);
		auto bussesIterB = busses.find(iter.first.connectionPointB.gateId);
		if (bussesIterA != busses.end() && bussesIterB != busses.end()) {
			auto busConnectionEndIdIterA = bussesIterA->second.connectionEnds.find(iter.first.connectionPointA.connectionEndId);
			assert(busConnectionEndIdIterA != bussesIterA->second.connectionEnds.end());
			auto busConnectionEndIdIterB = bussesIterB->second.connectionEnds.find(iter.first.connectionPointB.connectionEndId);
			assert(busConnectionEndIdIterB != bussesIterB->second.connectionEnds.end());
			assert(busConnectionEndIdIterA->second.size() == busConnectionEndIdIterB->second.size());
			for (unsigned int i = 0; i < busConnectionEndIdIterA->second.size(); i++) {
				auto busLaneJunctionIterA = bussesIterA->second.junctions.find(busConnectionEndIdIterA->second[i]);
				assert(busLaneJunctionIterA != bussesIterA->second.junctions.end());
				auto busLaneJunctionIterB = bussesIterB->second.junctions.find(busConnectionEndIdIterB->second[i]);
				assert(busLaneJunctionIterB != bussesIterB->second.junctions.end());
				nextState.addConnection(EvalConnection(
					EvalConnectionPoint(busLaneJunctionIterA->second, 0),
					EvalConnectionPoint(busLaneJunctionIterB->second, 0)
				), iter.second);
			}
		} else if (bussesIterA != busses.end()) {
			auto busConnectionEndIdIterA = bussesIterA->second.connectionEnds.find(iter.first.connectionPointA.connectionEndId);
			assert(busConnectionEndIdIterA != bussesIterA->second.connectionEnds.end());
			if (busConnectionEndIdIterA->second.size() == 1) { // bitwidth of 1
				auto busLaneJunctionIterA = bussesIterA->second.junctions.find(busConnectionEndIdIterA->second.front());
				assert(busLaneJunctionIterA != bussesIterA->second.junctions.end());
				assert(!busJunctions.contains(iter.first.connectionPointB.gateId)); // should not be a junction because there are not busJunctions of bitwidth 1
				nextState.addConnection(EvalConnection(
					EvalConnectionPoint(busLaneJunctionIterA->second, 0),
					iter.first.connectionPointB
				), iter.second);
			} else {
				auto busJunctionsIterPair = busJunctions.try_emplace(iter.first.connectionPointB.gateId);
				if (!busJunctionsIterPair.second) {
					const EvalGate* gateB = currentState.getGate(iter.first.connectionPointB.gateId);
					assert(isJunctionType(gateB->type)); // nothing else thats not a bus of junction takes and non 1 bitwidth
					assert(gateB->connections.empty()); // junctions should be merged it has no connections
					busJunctionsIterPair.first->second.push_back(iter.first.connectionPointB.gateId);
					for (unsigned int i = 1; i < busConnectionEndIdIterA->second.size(); i++) {
						eval_gate_id junctionId = nextState.getUnusedEvalGateId();
						busJunctionsIterPair.first->second.push_back(junctionId);
						nextState.addGate(junctionId, gateB->type);
					}
				}
				for (unsigned int i = 0; i < busConnectionEndIdIterA->second.size(); i++) {
					auto busLaneJunctionIterA = bussesIterA->second.junctions.find(busConnectionEndIdIterA->second[i]);
					assert(busLaneJunctionIterA != bussesIterA->second.junctions.end());
					nextState.addConnection(EvalConnection(
						EvalConnectionPoint(busLaneJunctionIterA->second, 0),
						EvalConnectionPoint(busJunctionsIterPair.first->second[i], 0)
					), iter.second);
				}
			}
		} else if (bussesIterB != busses.end()) {
			auto busConnectionEndIdIterB = bussesIterB->second.connectionEnds.find(iter.first.connectionPointB.connectionEndId);
			assert(busConnectionEndIdIterB != bussesIterB->second.connectionEnds.end());
			if (busConnectionEndIdIterB->second.size() == 1) { // bitwidth of 1
				auto busLaneJunctionIterB = bussesIterB->second.junctions.find(busConnectionEndIdIterB->second.front());
				assert(busLaneJunctionIterB != bussesIterB->second.junctions.end());
				assert(!busJunctions.contains(iter.first.connectionPointA.gateId)); // should not be a junction because there are not busJunctions of bitwidth 1
				nextState.addConnection(EvalConnection(
					EvalConnectionPoint(busLaneJunctionIterB->second, 0),
					iter.first.connectionPointA
				), iter.second);
			} else {
				auto busJunctionsIterPair = busJunctions.try_emplace(iter.first.connectionPointA.gateId);
				if (busJunctionsIterPair.second) {
					const EvalGate* gateB = currentState.getGate(iter.first.connectionPointA.gateId);
					assert(isJunctionType(gateB->type)); // nothing else thats not a bus of junction takes and non 1 bitwidth
					// assert(gateB->connections.empty()); // junctions should be merged it has no connections
					busJunctionsIterPair.first->second.push_back(iter.first.connectionPointA.gateId);
					for (unsigned int i = 1; i < busConnectionEndIdIterB->second.size(); i++) {
						eval_gate_id junctionId = nextState.getUnusedEvalGateId();
						busJunctionsIterPair.first->second.push_back(junctionId);
						nextState.addGate(junctionId, gateB->type);
					}
				}
				for (unsigned int i = 0; i < busConnectionEndIdIterB->second.size(); i++) {
					auto busLaneJunctionIterB = bussesIterB->second.junctions.find(busConnectionEndIdIterB->second[i]);
					assert(busLaneJunctionIterB != bussesIterB->second.junctions.end());
					nextState.addConnection(EvalConnection(
						EvalConnectionPoint(busLaneJunctionIterB->second, 0),
						EvalConnectionPoint(busJunctionsIterPair.first->second[i], 0)
					), iter.second);
				}
			}
		} else {
			nextState.addConnection(iter.first, iter.second);
		}
	}
	for (eval_gate_id gateId : currentState.getGateIdRemappingsUpdateds()) {
		nextState.addGateIdRemappingsUpdated(gateId);
	}
	for (EvalConnectionPoint connectionPoint : currentState.getConnectionPointRemappingsUpdated()) {
		nextState.addConnectionPointRemappingsUpdated(connectionPoint);
	}
}
