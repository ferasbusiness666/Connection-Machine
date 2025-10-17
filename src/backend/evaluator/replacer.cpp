#include "replacer.h"
#include "util/algorithm.h"

Replacement& Replacer::makeReplacement(int layer) {
	replacements.push_back(Replacement(
		this,
		layer
	));
	return replacements.back();
}

void Replacer::cleanReplacements() {
	for (auto it = replacements.begin(); it != replacements.end();) {
		if (it->getIsEmpty()) {
			it = replacements.erase(it);
		} else {
			++it;
		}
	}
}

void Replacer::pingOutputs(SimPauseGuard& pauseGuard, middle_id_t id) {
	for (Replacement& replacement : replacements) {
		replacement.pingOutput(pauseGuard, id);
	}
}

void Replacer::pingInputs(SimPauseGuard& pauseGuard, middle_id_t id) {
	for (Replacement& replacement : replacements) {
		replacement.pingInput(pauseGuard, id);
	}
}

EvalConnectionPoint Replacer::getReplacementConnectionPoint(EvalConnectionPoint point) const {
	if (replacedIds.contains(point.gateId)) {
		EvalConnectionPoint newPoint = EvalConnectionPoint(replacedIds.at(point.gateId), point.portId);
		if (newPoint != point) {
			return getReplacementConnectionPoint(newPoint);
		}
		return newPoint;
	}
	if (replacedConnectionPoints.contains(point.gateId) && replacedConnectionPoints.at(point.gateId).contains(point.portId)) {
		EvalConnectionPoint newPoint = replacedConnectionPoints.at(point.gateId).at(point.portId);
		if (newPoint != point) {
			return getReplacementConnectionPoint(newPoint);
		}
		return newPoint;
	}
	return point;
}

std::vector<EvalConnectionPoint> Replacer::getReplacementConnectionPoints(const std::vector<EvalConnectionPoint>& points) const {
	std::vector<EvalConnectionPoint> result;
	result.reserve(points.size());
	for (const EvalConnectionPoint& point : points) {
		result.push_back(getReplacementConnectionPoint(point));
	}
	return result;
}

std::vector<std::optional<EvalConnectionPoint>> Replacer::getReplacementConnectionPoints(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
	std::vector<std::optional<EvalConnectionPoint>> result;
	result.reserve(points.size());
	for (const std::optional<EvalConnectionPoint>& point : points) {
		if (!point.has_value()) {
			result.push_back(std::nullopt);
			continue;
		}
		result.push_back(getReplacementConnectionPoint(point.value()));
	}
	return result;
}

void Replacer::mergeBuses(SimPauseGuard& pauseGuard, int layer) {
	std::vector<middle_id_t> allMiddleIds = middleIdProvider.getUsedIds();
	for (const middle_id_t id : allMiddleIds) {
		BlockType blockType = busInterfacePassthrough.getBlockType(id);
		if (blockType == BlockType::NONE) {
			continue;
		}
		const BlockData* blockData = blockDataManager.getBlockData(blockType);
		if (!blockData->isBus()) {
			continue;
		}
		logInfo("Merging bus {}", "Replacer::mergeBuses", id);
		const std::vector<EvalConnection>& outputs = busInterfacePassthrough.getOutputs(id);
		const std::vector<EvalConnection>& inputs = busInterfacePassthrough.getInputs(id);
		const std::unordered_map<connection_end_id_t, BlockData::ConnectionData>& connectionMap = blockData->getConnections();
		for (const auto& [connectionId, connectionData] : connectionMap) {
			unsigned int bitWidth = connectionData.getBitWidth();
			if (bitWidth != 1) {
				continue;
			}
			if (connectionPointBusOverride.contains(EvalConnectionPoint(id, connectionId))) {
				continue; // already overridden by another bus
			}
			unsigned int laneId;
			std::variant<unsigned int, std::vector<unsigned int>> bitConfiguration = connectionData.bitConfiguration;
			if (std::holds_alternative<unsigned int>(bitConfiguration)) {
				logError("Bus {} connection has single bit width but non-vector bit configuration > 1", "Replacer::mergeBuses", id);
				continue;
			} else {
				laneId = std::get<std::vector<unsigned int>>(bitConfiguration).at(0);
			}
			Replacement& replacement = makeReplacement(layer);
			middle_id_t newJunctionId = replacement.getNewId();
			LanePointSet visitedLanePoints;
			mergeBusLane(pauseGuard, replacement, id, laneId, newJunctionId, visitedLanePoints);
		}
	}
}

void Replacer::mergeBusLane(SimPauseGuard& pauseGuard, Replacement& replacement, middle_id_t busId, unsigned int laneId, middle_id_t newJunctionId, LanePointSet& visitedLanePoints) {
	LanePoint lanePoint = { busId, laneId };
	if (visitedLanePoints.contains(lanePoint)) {
		return;
	}
	visitedLanePoints.insert(lanePoint);
	logInfo("Merging bus lane {}:{}", "Replacer::mergeBusLane", busId, laneId);
	const BlockData* blockData = blockDataManager.getBlockData(busInterfacePassthrough.getBlockType(busId));
	if (blockData == nullptr) {
		logError("Bus {} has invalid block type", "Replacer::mergeBusLane", busId);
		return;
	}
	const std::unordered_map<connection_end_id_t, BlockData::ConnectionData>& connectionMap = blockData->getConnections();
	for (const auto& [connectionId, connectionData] : connectionMap) {
		unsigned int bitWidth = connectionData.getBitWidth();
		if (bitWidth == 1) {
			unsigned int connectionLaneId = connectionData.getFirstLaneId();
			if (connectionLaneId != laneId) {
				continue;
			}
			EvalConnectionPoint connectionPoint = EvalConnectionPoint(busId, connectionId);
			addConnectionPointBusOverride(busId, connectionPoint, EvalConnectionPoint(newJunctionId, 0), replacement);
		} else {
			std::vector<unsigned int> connectionLaneIds = connectionData.getLaneIds();
			for (unsigned int i = 0; i < connectionLaneIds.size(); i++) {
				if (connectionLaneIds.at(i) != laneId) {
					continue;
				}
				EvalConnectionPoint connectionPoint = EvalConnectionPoint(busId, connectionId);
				// need to recursively merge other buses connected to this connection point
			}
		}
	}
}

void Replacer::mergeJunctions(SimPauseGuard& pauseGuard, int layer) {
	std::vector<middle_id_t> allMiddleIds = middleIdProvider.getUsedIds();
	for (const middle_id_t id : allMiddleIds) {
		if (replacementIdLayers.contains(id)) {
			if (replacementIdLayers.at(id) >= layer) {
				continue;
			}
		}
		// check if we're a junction
		BlockType blockType = busInterfacePassthrough.getBlockType(id);
		if (blockType != BlockType::JUNCTION) {
			continue;
		}
		JunctionFloodFillResult floodFillResult = junctionFloodFill(id);

		if (floodFillResult.outputsGoingIntoJunctions.size() == 0) {
			continue;
		}

		Replacement& replacement = makeReplacement(layer);
		if (floodFillResult.outputsGoingIntoJunctions.size() == 1) {
			EvalConnectionPoint output = floodFillResult.outputsGoingIntoJunctions.at(0);
			for (const middle_id_t junctionId : floodFillResult.junctionIds) {
				replacement.removeGate(pauseGuard, junctionId, { {0, output }, {1, output } });
			}
			for (const EvalConnection& input : floodFillResult.inputsPullingFromJunctions) {
				replacement.makeConnection(pauseGuard, EvalConnection(output, input.destination));
			}
			replacement.trackOutput(output.gateId);
		} else {
			middle_id_t newJunctionId = replacement.getNewId();
			replacement.addGate(pauseGuard, BlockType::JUNCTION, newJunctionId);
			for (const middle_id_t junctionId : floodFillResult.junctionIds) {
				replacement.removeGate(pauseGuard, junctionId, newJunctionId);
			}
			for (const EvalConnection& input : floodFillResult.inputsPullingFromJunctions) {
				replacement.makeConnection(pauseGuard, EvalConnection(EvalConnectionPoint(newJunctionId, 0), input.destination));
			}
			for (const EvalConnectionPoint& output : floodFillResult.outputsGoingIntoJunctions) {
				replacement.makeConnection(pauseGuard, EvalConnection(output, EvalConnectionPoint(newJunctionId, 0)));
			}
			for (const EvalConnection& conn : floodFillResult.connectionsToReroute) {
				EvalConnection newConnection = EvalConnection(EvalConnectionPoint(newJunctionId, 0), conn.destination);
				replacement.removeConnection(pauseGuard, conn);
				replacement.makeConnection(pauseGuard, newConnection);
			}
		}
	}
}

Replacer::JunctionFloodFillResult Replacer::junctionFloodFill(middle_id_t junctionId) {
	JunctionFloodFillResult result;
	std::set<middle_id_t> visited;
	std::set<EvalConnectionPoint> visitedOutputs;
	std::queue<middle_id_t> queue;
	queue.push(junctionId);
	visited.insert(junctionId);
	while (!queue.empty()) {
		middle_id_t currentId = queue.front();
		queue.pop();
		result.junctionIds.push_back(currentId);
		std::vector<EvalConnection> outputs = busInterfacePassthrough.getOutputs(currentId);
		std::vector<EvalConnection> inputs = busInterfacePassthrough.getInputs(currentId);
		for (const EvalConnection& output : outputs) {
			if (visited.contains(output.destination.gateId)) {
				continue;
			}
			BlockType outputBlockType = busInterfacePassthrough.getBlockType(output.destination.gateId);
			if (outputBlockType == BlockType::JUNCTION) {
				queue.push(output.destination.gateId);
				visited.insert(output.destination.gateId);
				continue;
			}
			result.inputsPullingFromJunctions.push_back(output);
		}
		for (const EvalConnection& input : inputs) {
			if (visited.contains(input.source.gateId)) {
				continue;
			}
			BlockType inputBlockType = busInterfacePassthrough.getBlockType(input.source.gateId);
			if (inputBlockType == BlockType::JUNCTION) {
				queue.push(input.source.gateId);
				visited.insert(input.source.gateId);
				continue;
			}
			// not a junction going into a junction
			if (visitedOutputs.contains(input.source)) {
				continue;
			}
			visitedOutputs.insert(input.source);
			result.outputsGoingIntoJunctions.push_back(input.source);
			std::vector<EvalConnection> nodeOutputs = busInterfacePassthrough.getOutputs(input.source.gateId);
			for (const EvalConnection& nodeOutput : nodeOutputs) {
				if (nodeOutput.source.portId != input.source.portId) {
					continue; // only consider outputs from the same port
				}
				BlockType nodeOutputBlockType = busInterfacePassthrough.getBlockType(nodeOutput.destination.gateId);
				if (nodeOutputBlockType == BlockType::JUNCTION) {
					if (visited.contains(nodeOutput.destination.gateId)) {
						continue;
					}
					queue.push(nodeOutput.destination.gateId);
					visited.insert(nodeOutput.destination.gateId);
				} else {
					result.connectionsToReroute.push_back(nodeOutput);
				}
			}
		}
	}
	return result;
}
