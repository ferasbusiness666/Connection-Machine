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
		const BlockData* blockData = circuitManager.getBlockDataManager().getBlockData(getBlockType(iter.second));
		assert(blockData);
		if (blockData->isBus()) {
			std::unordered_map<unsigned int, eval_gate_id> lanes;
			for (std::pair<connection_end_id_t, BlockData::ConnectionData> connectionData : blockData->getConnectionsSafe()) {
				const std::variant<unsigned int, std::vector<unsigned int>>& bitConfiguration = connectionData.second.bitConfiguration;
				if (std::holds_alternative<unsigned int>(bitConfiguration)) {
					for (unsigned int lane = 0; lane < std::get<unsigned int>(bitConfiguration); lane++) {
						auto pair = lanes.emplace(lane, 0);
						if (pair.second) {
							auto remappingIter = nextState.getConnectionPointRemapping().find(EvalConnectionPoint(iter.first, connectionData.first));
							assert(remappingIter != nextState.getConnectionPointRemapping().end());
							pair.first->second = remappingIter->second.gateId;
							nextState.removeGate(remappingIter->second.gateId);
						}
					}
				} else {
					for (unsigned int lane : std::get<std::vector<unsigned int>>(bitConfiguration)) {
						auto pair = lanes.emplace(lane, 0);
					}
				}
			}
			for (std::pair<unsigned int, eval_gate_id> lane : lanes) {
				auto iterPair = nextState.getConnectionPointReverseRemapping().equal_range(EvalConnectionPoint(lane.second, 0));
				for (auto reverseRemappingIter = iterPair.first; reverseRemappingIter != iterPair.second; reverseRemappingIter++) {
					nextState.getConnectionPointRemapping().erase(reverseRemappingIter->second);
				}
				nextState.getConnectionPointReverseRemapping().erase(iterPair.first, iterPair.second);
			}
		} else {
			nextState.getGateIdRemapping().erase(iter.first);
			nextState.getGateIdReverseRemapping().erase(iter.first);
			nextState.removeGate(iter.first);
		}
	}
	for (auto iter : currentState.getAddedGates()) {
		const BlockData* blockData = circuitManager.getBlockDataManager().getBlockData(getBlockType(iter.second));
		assert(blockData);
		if (blockData->isBus()) {
			std::unordered_map<unsigned int, eval_gate_id> lanes;
			for (std::pair<connection_end_id_t, BlockData::ConnectionData> connectionData : blockData->getConnectionsSafe()) {
				const std::variant<unsigned int, std::vector<unsigned int>>& bitConfiguration = connectionData.second.bitConfiguration;
				if (std::holds_alternative<unsigned int>(bitConfiguration)) {
					for (unsigned int lane = 0; lane < std::get<unsigned int>(bitConfiguration); lane++) {
						auto pair = lanes.emplace(lane, 0);
						if (pair.second) {
							eval_gate_id junctionId = nextState.getUnusedEvalGateId();
							pair.first->second = junctionId;
							nextState.addGate(junctionId, getEvalGateType(BlockType::JUNCTION));
							nextState.getConnectionPointRemapping().emplace(
								EvalConnectionPoint(iter.first, connectionData.first),
								EvalConnectionPoint(junctionId, 0)
							);
							nextState.getConnectionPointReverseRemapping().emplace(
								EvalConnectionPoint(junctionId, 0),
								EvalConnectionPoint(iter.first, connectionData.first)
							);
						} else {
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
				} else {
					for (unsigned int lane : std::get<std::vector<unsigned int>>(bitConfiguration)) {
						auto pair = lanes.emplace(lane, 0);
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
		EvalConnection connection = iter.first;
		auto remappingIterA = nextState.getConnectionPointRemapping().find(connection .connectionPointA);
		if (remappingIterA != nextState.getConnectionPointRemapping().end()) {
			connection.connectionPointA = remappingIterA->second;
		}
		auto remappingIterB = nextState.getConnectionPointRemapping().find(connection .connectionPointB);
		if (remappingIterB != nextState.getConnectionPointRemapping().end()) {
			connection.connectionPointB = remappingIterB->second;
		}
		nextState.addConnection(connection, iter.second);
	}
	for (eval_gate_id gateId : currentState.getGateIdRemappingsUpdateds()) {
		nextState.addGateIdRemappingsUpdated(gateId);
	}
	for (EvalConnectionPoint connectionPoint : currentState.getConnectionPointRemappingsUpdated()) {
		nextState.addConnectionPointRemappingsUpdated(connectionPoint);
	}
}
