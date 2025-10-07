#include "replacer.h"
#include "util/algorithm.h"

Replacement& Replacer::makeReplacement(int layer) {
	replacements.push_back(Replacement(
		this,
		&busInterfacePassthrough,
		&middleIdProvider,
		&replacedIds,
		&replacedConnectionPoints,
		&replacementIdLayers,
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
		GateType gateType = busInterfacePassthrough.getGateType(id);
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
		GateType gateType = busInterfacePassthrough.getGateType(id);
		if (gateType != GateType::JUNCTION) {
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
			replacement.addGate(pauseGuard, GateType::JUNCTION, newJunctionId);
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
			GateType outputGateType = busInterfacePassthrough.getGateType(output.destination.gateId);
			if (outputGateType == GateType::JUNCTION) {
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
			GateType inputGateType = busInterfacePassthrough.getGateType(input.source.gateId);
			if (inputGateType == GateType::JUNCTION) {
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
				GateType nodeOutputGateType = busInterfacePassthrough.getGateType(nodeOutput.destination.gateId);
				if (nodeOutputGateType == GateType::JUNCTION) {
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