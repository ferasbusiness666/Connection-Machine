#include "replacement.h"
#include "logicSimulator.h"

#include "replacer.h"

void Replacement::removeGate(
	SimPauseGuard& pauseGuard,
	middle_id_t gateId,
	std::unordered_map<connection_port_id_t, EvalConnectionPoint> replacementConnectionPoints) {
	isEmpty = false;
	// track connection removals
	std::vector<EvalConnection> outputs = replacer->busInterfacePassthrough.getOutputs(gateId);
	std::vector<EvalConnection> inputs = replacer->busInterfacePassthrough.getInputs(gateId);
	for (const auto& conn : outputs) {
		if (conn.destination.gateId != conn.source.gateId) {
			deletedConnections.push_back(conn);
		}
	}
	for (const auto& conn : inputs) {
		deletedConnections.push_back(conn);
	}
	deletedGates.push_back({ gateId, replacer->busInterfacePassthrough.getGateType(gateId) });
	idsToTrackInputs.insert(gateId);
	idsToTrackOutputs.insert(gateId);
	replacer->replacedConnectionPoints.insert({ gateId, replacementConnectionPoints });
	replacer->busInterfacePassthrough.removeGate(pauseGuard, gateId);
}

void Replacement::removeGate(
	SimPauseGuard& pauseGuard,
	middle_id_t gateId,
	middle_id_t replacementId) {
	isEmpty = false;
	// track connection removals
	std::vector<EvalConnection> outputs = replacer->busInterfacePassthrough.getOutputs(gateId);
	std::vector<EvalConnection> inputs = replacer->busInterfacePassthrough.getInputs(gateId);
	for (const auto& conn : outputs) {
		if (conn.destination.gateId != conn.source.gateId) {
			deletedConnections.push_back(conn);
		}
	}
	for (const auto& conn : inputs) {
		deletedConnections.push_back(conn);
	}
	deletedGates.push_back({ gateId, replacer->busInterfacePassthrough.getGateType(gateId) });
	idsToTrackInputs.insert(gateId);
	idsToTrackOutputs.insert(gateId);
	replacer->replacedIds.insert({ gateId, replacementId });
	replacer->busInterfacePassthrough.removeGate(pauseGuard, gateId);
	replacer->replacementIdLayers.insert({ replacementId, layer });
}

void Replacement::addGate(
	SimPauseGuard& pauseGuard,
	GateType gateType,
	middle_id_t gateId) {
	isEmpty = false;
	replacer->busInterfacePassthrough.addGate(pauseGuard, gateType, gateId);
	// we don't need to track, because nothing can happen to this gate at a lower level
	addedGates.push_back({ gateId, gateType });
}

void Replacement::removeConnection(
	SimPauseGuard& pauseGuard,
	EvalConnection connection) {
	isEmpty = false;
	replacer->busInterfacePassthrough.removeConnection(pauseGuard, connection);
	idsToTrackInputs.insert(connection.destination.gateId);
	idsToTrackOutputs.insert(connection.source.gateId);
	deletedConnections.push_back(connection);
}

void Replacement::makeConnection(
	SimPauseGuard& pauseGuard,
	EvalConnection connection) {
	isEmpty = false;
	replacer->busInterfacePassthrough.makeConnection(pauseGuard, connection);
	idsToTrackInputs.insert(connection.destination.gateId);
	idsToTrackOutputs.insert(connection.source.gateId);
	addedConnections.push_back(connection);
}

middle_id_t Replacement::getNewId() {
	middle_id_t newId = replacer->middleIdProvider.getNewId();
	reservedIds.push_back(newId);
	return newId;
}

void Replacement::revert(SimPauseGuard& pauseGuard) {
	if (isReverting) {
		return;
	}
	isReverting = true;
	isEmpty = true;
	for (const auto& conn : addedConnections) {
		replacer->pingOutputs(pauseGuard, conn.source.gateId);
		replacer->pingInputs(pauseGuard, conn.destination.gateId);
	}
	for (const auto& conn : deletedConnections) {
		replacer->pingOutputs(pauseGuard, conn.source.gateId);
		replacer->pingInputs(pauseGuard, conn.destination.gateId);
	}
	for (const auto& conn : addedGates) {
		replacer->pingOutputs(pauseGuard, conn.id);
		replacer->pingInputs(pauseGuard, conn.id);
	}
	for (const auto& conn : addedConnections) {
		replacer->busInterfacePassthrough.removeConnection(pauseGuard, conn);
	}
	for (const auto& conn : addedGates) {
		replacer->busInterfacePassthrough.removeGate(pauseGuard, conn.id);
	}
	for (const auto& gate : deletedGates) {
		replacer->busInterfacePassthrough.addGate(pauseGuard, gate.type, gate.id);
		replacer->replacedConnectionPoints.erase(gate.id);
		replacer->replacedIds.erase(gate.id);
	}
	for (const auto& conn : deletedConnections) {
		replacer->busInterfacePassthrough.makeConnection(pauseGuard, conn);
	}
	for (const auto& id : reservedIds) {
		replacer->removeconnectionPointBusOverride(id);
		replacer->middleIdProvider.releaseId(id);
		replacer->replacementIdLayers.erase(id);
	}
	addedConnections.clear();
	addedGates.clear();
	deletedConnections.clear();
	deletedGates.clear();
	reservedIds.clear();
	idsToTrackInputs.clear();
	idsToTrackOutputs.clear();
	isReverting = false;
}
