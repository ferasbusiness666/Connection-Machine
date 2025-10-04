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
    for (auto& replacement : replacements) {
        replacement.pingOutput(pauseGuard, id);
    }
}

void Replacer::pingInputs(SimPauseGuard& pauseGuard, middle_id_t id) {
    for (auto& replacement : replacements) {
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
    for (const auto& point : points) {
        result.push_back(getReplacementConnectionPoint(point));
    }
    return result;
}

std::vector<std::optional<EvalConnectionPoint>> Replacer::getReplacementConnectionPoints(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
    std::vector<std::optional<EvalConnectionPoint>> result;
    result.reserve(points.size());
    for (const auto& point : points) {
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
        if (replacementIdLayers.contains(id)) {
            if (replacementIdLayers.at(id) >= layer) {
                continue;
            }
        }
        // check if we're a bus interface
        GateType gateType = busInterfacePassthrough.getGateType(id);
        if (gateType != GateType::BUS_INTERFACE) {
            continue;
        }
        BusFloodFillResult floodFillResult = busFloodFill(id);
        logInfo("Merging " + std::to_string(floodFillResult.busIds.size()) + " bus interfaces and " + std::to_string(floodFillResult.junctionIds.size()) + " junctions.", "Replacer::mergeBuses");
        Replacement& replacement = makeReplacement(layer);
        std::unordered_set<middle_id_t> busesMet;
        for (const middle_id_t busId : floodFillResult.busIds) {
            busesMet.insert(busId);
        }
        for (const EvalConnection& conn : floodFillResult.connectionsBetweenBusesAndJunctions) {
            replacement.removeConnection(pauseGuard, conn);
        }
        for (const middle_id_t junctionId : floodFillResult.junctionIds) {
            replacement.removeGate(pauseGuard, junctionId, 0);
        }
        std::vector<middle_id_t> pinJunctionIds = {};
        std::vector<EvalConnection> sameBusConnections = {};
        for (const EvalConnection& conn : floodFillResult.connectionsIntoBuses) {
            int pin = conn.destination.portId / 2 - 1;
            while (pin + 1 > pinJunctionIds.size()) {
                middle_id_t newJunctionId = replacement.getNewId();
                replacement.addGate(pauseGuard, GateType::JUNCTION, newJunctionId);
                pinJunctionIds.push_back(newJunctionId);
                logInfo("Created junction " + std::to_string(newJunctionId) + " for bus pin " + std::to_string(pin), "Replacer::mergeBuses");
            }
            if (busesMet.contains(conn.source.gateId)) {
                sameBusConnections.push_back(conn);
                continue;
            }
            middle_id_t junctionId = pinJunctionIds[pin];
            replacement.removeConnection(pauseGuard, conn);
            EvalConnection newConnection = EvalConnection(conn.source, EvalConnectionPoint(junctionId, 0));
            replacement.makeConnection(pauseGuard, newConnection);
            logInfo("Rerouted connection from " + conn.toString() + " to " + newConnection.toString(), "Replacer::mergeBuses");
        }
        for (const EvalConnection& conn : floodFillResult.connectionsOutOfBuses) {
            int pin = conn.source.portId / 2 - 1;
            while (pin + 1 > pinJunctionIds.size()) {
                middle_id_t newJunctionId = replacement.getNewId();
                replacement.addGate(pauseGuard, GateType::JUNCTION, newJunctionId);
                pinJunctionIds.push_back(newJunctionId);
                logInfo("Created junction " + std::to_string(newJunctionId) + " for bus pin " + std::to_string(pin), "Replacer::mergeBuses");
            }
            if (busesMet.contains(conn.destination.gateId)) {
                continue;
            }
            middle_id_t junctionId = pinJunctionIds[pin];
            replacement.removeConnection(pauseGuard, conn);
            EvalConnection newConnection = EvalConnection(EvalConnectionPoint(junctionId, 0), conn.destination);
            replacement.makeConnection(pauseGuard, newConnection);
            logInfo("Rerouted connection from " + conn.toString() + " to " + newConnection.toString(), "Replacer::mergeBuses");
        }
        for (EvalConnection& conn : sameBusConnections) {
            replacement.removeConnection(pauseGuard, conn);
            int pinSource = conn.source.portId / 2 - 1;
            int pinDest = conn.destination.portId / 2 - 1;
            middle_id_t junctionIdSource = pinJunctionIds[pinSource];
            middle_id_t junctionIdDest = pinJunctionIds[pinDest];
            EvalConnection newConnection = EvalConnection(EvalConnectionPoint(junctionIdSource, 0), EvalConnectionPoint(junctionIdDest, 0));
            replacement.makeConnection(pauseGuard, newConnection);
            logInfo("Rerouted connection from " + conn.toString() + " to " + newConnection.toString(), "Replacer::mergeBuses");
        }
        std::unordered_map<connection_port_id_t, EvalConnectionPoint> pinsMapping = {};
        pinsMapping[0] = EvalConnectionPoint(0, 0);
        pinsMapping[1] = EvalConnectionPoint(0, 0);
        for (size_t i = 0; i < pinJunctionIds.size(); i++) {
            pinsMapping[i * 2 + 2] = EvalConnectionPoint(pinJunctionIds[i], 0);
            pinsMapping[i * 2 + 3] = EvalConnectionPoint(pinJunctionIds[i], 0);
        }
        for (const middle_id_t busId : floodFillResult.busIds) {
            replacement.removeGate(pauseGuard, busId, pinsMapping);
        }
    }
}

Replacer::BusFloodFillResult Replacer::busFloodFill(middle_id_t busId) {
    BusFloodFillResult result;
    std::set<middle_id_t> visited;
    std::queue<middle_id_t> queue;
    queue.push(busId);
    visited.insert(busId);
    while (!queue.empty()) {
        middle_id_t currentId = queue.front();
        queue.pop();
        GateType gateType = busInterfacePassthrough.getGateType(currentId);
        if (gateType == GateType::BUS_INTERFACE) {
            result.busIds.push_back(currentId);
        } else if (gateType == GateType::JUNCTION) {
            result.junctionIds.push_back(currentId);
        } else {
            logError("Bus flood fill encountered non-bus, non-junction gate. This should not happen.", "Replacer::busFloodFill");
            continue;
        }
        std::vector<EvalConnection> outputs = busInterfacePassthrough.getOutputs(currentId);
        std::vector<EvalConnection> inputs = busInterfacePassthrough.getInputs(currentId);
        for (const auto& output : outputs) {
            GateType outputGateType = busInterfacePassthrough.getGateType(output.destination.gateId);
            bool isBusPort = (output.source.portId < 2 && (gateType == GateType::BUS_INTERFACE || gateType == GateType::JUNCTION));
            if (isBusPort) {
                if (visited.contains(output.destination.gateId)) {
                    if (!contains(
                        result.connectionsBetweenBusesAndJunctions.begin(),
                        result.connectionsBetweenBusesAndJunctions.end(),
                        output
                    )) {
                        result.connectionsBetweenBusesAndJunctions.push_back(output);
                    }
                    continue;
                }
                queue.push(output.destination.gateId);
                visited.insert(output.destination.gateId);
                result.connectionsBetweenBusesAndJunctions.push_back(output);
                continue;
            }
            result.connectionsOutOfBuses.push_back(output);
        }
        for (const auto& input : inputs) {
            GateType inputGateType = busInterfacePassthrough.getGateType(input.source.gateId);
            bool isBusPort = (input.destination.portId < 2 && (gateType == GateType::BUS_INTERFACE || gateType == GateType::JUNCTION));
            if (isBusPort) {
                if (visited.contains(input.source.gateId)) {
                    if (!contains(
                        result.connectionsBetweenBusesAndJunctions.begin(),
                        result.connectionsBetweenBusesAndJunctions.end(),
                        input
                    )) {
                        result.connectionsBetweenBusesAndJunctions.push_back(input);
                    }
                    continue;
                }
                queue.push(input.source.gateId);
                visited.insert(input.source.gateId);
                result.connectionsBetweenBusesAndJunctions.push_back(input);
                continue;
            }
            result.connectionsIntoBuses.push_back(input);
        }
    }
    return result;
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
            for (const auto& junctionId : floodFillResult.junctionIds) {
                replacement.removeGate(pauseGuard, junctionId, { {0, output }, {1, output } });
            }
            for (const auto& input : floodFillResult.inputsPullingFromJunctions) {
                replacement.makeConnection(pauseGuard, EvalConnection(output, input.destination));
            }
            replacement.trackOutput(output.gateId);
        } else {
            middle_id_t newJunctionId = replacement.getNewId();
            replacement.addGate(pauseGuard, GateType::JUNCTION, newJunctionId);
            for (const auto& junctionId : floodFillResult.junctionIds) {
                replacement.removeGate(pauseGuard, junctionId, newJunctionId);
            }
            for (const auto& input : floodFillResult.inputsPullingFromJunctions) {
                replacement.makeConnection(pauseGuard, EvalConnection(EvalConnectionPoint(newJunctionId, 0), input.destination));
            }
            for (const auto& output : floodFillResult.outputsGoingIntoJunctions) {
                replacement.makeConnection(pauseGuard, EvalConnection(output, EvalConnectionPoint(newJunctionId, 0)));
            }
            for (const auto& conn : floodFillResult.connectionsToReroute) {
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
        for (const auto& output : outputs) {
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
        for (const auto& input : inputs) {
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
            for (const auto& nodeOutput : nodeOutputs) {
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