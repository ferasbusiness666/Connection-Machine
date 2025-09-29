#include "replacement.h"
#include "logicSimulator.h"

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
		busInterfacePassthrough->removeConnection(pauseGuard, conn);
	}
	for (const auto& conn : addedGates) {
		busInterfacePassthrough->removeGate(pauseGuard, conn.id);
	}
	for (const auto& gate : deletedGates) {
		busInterfacePassthrough->addGate(pauseGuard, gate.type, gate.id);
		replacedConnectionPoints->erase(gate.id);
		replacedIds->erase(gate.id);
	}
	for (const auto& conn : deletedConnections) {
		busInterfacePassthrough->makeConnection(pauseGuard, conn);
	}
	for (const auto& id : reservedIds) {
		middleIdProvider->releaseId(id);
		replacementIds->erase(id);
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
