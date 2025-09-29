#ifndef replacement_h
#define replacement_h

#include "busInterfacePassthrough.h"
#include "evalConnection.h"
#include "evalTypedef.h"
#include "idProvider.h"
#include "gateType.h"

class Replacer;

struct ReplacementGate {
    middle_id_t id;
	GateType type;
};

class Replacement {
public:
	Replacement(
		Replacer* replacer,
		BusInterfacePassthrough* optimizer,
		IdProvider<middle_id_t>* middleIdProvider,
		std::unordered_map<middle_id_t, middle_id_t>* replacedIds,
		std::unordered_map<middle_id_t, std::unordered_map<connection_port_id_t, EvalConnectionPoint>>* replacedConnectionPoints,
		std::unordered_set<middle_id_t>* replacementIds
	) :
		replacer(replacer),
		busInterfacePassthrough(optimizer),
		middleIdProvider(middleIdProvider),
		replacedIds(replacedIds),
		replacedConnectionPoints(replacedConnectionPoints),
		replacementIds(replacementIds) {}

	void removeGate(SimPauseGuard& pauseGuard, middle_id_t gateId, std::unordered_map<connection_port_id_t, EvalConnectionPoint> replacementConnectionPoints) {
		isEmpty = false;
		// track connection removals
		std::vector<EvalConnection> outputs = busInterfacePassthrough->getOutputs(gateId);
		std::vector<EvalConnection> inputs = busInterfacePassthrough->getInputs(gateId);
		for (const auto& conn : outputs) {
			if (conn.destination.gateId != conn.source.gateId) {
				deletedConnections.push_back(conn);
			}
		}
		for (const auto& conn : inputs) {
			deletedConnections.push_back(conn);
		}
		deletedGates.push_back({ gateId, busInterfacePassthrough->getGateType(gateId) });
		idsToTrackInputs.insert(gateId);
		idsToTrackOutputs.insert(gateId);
		replacedConnectionPoints->insert({ gateId, replacementConnectionPoints });
		busInterfacePassthrough->removeGate(pauseGuard, gateId);
	}

	void removeGate(SimPauseGuard& pauseGuard, middle_id_t gateId, middle_id_t replacementId) {
		isEmpty = false;
		// track connection removals
		std::vector<EvalConnection> outputs = busInterfacePassthrough->getOutputs(gateId);
		std::vector<EvalConnection> inputs = busInterfacePassthrough->getInputs(gateId);
		for (const auto& conn : outputs) {
			if (conn.destination.gateId != conn.source.gateId) {
				deletedConnections.push_back(conn);
			}
		}
		for (const auto& conn : inputs) {
			deletedConnections.push_back(conn);
		}
		deletedGates.push_back({ gateId, busInterfacePassthrough->getGateType(gateId) });
		idsToTrackInputs.insert(gateId);
		idsToTrackOutputs.insert(gateId);
		replacedIds->insert({ gateId, replacementId });
		busInterfacePassthrough->removeGate(pauseGuard, gateId);
		replacementIds->insert(replacementId);
	}

	void addGate(SimPauseGuard& pauseGuard, GateType gateType, middle_id_t gateId) {
		isEmpty = false;
		busInterfacePassthrough->addGate(pauseGuard, gateType, gateId);
		// we don't need to track, because nothing can happen to this gate
		addedGates.push_back({ gateId, gateType });
	}

	void removeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
		isEmpty = false;
		busInterfacePassthrough->removeConnection(pauseGuard, connection);
		idsToTrackInputs.insert(connection.destination.gateId);
		idsToTrackOutputs.insert(connection.source.gateId);
		deletedConnections.push_back(connection);
	}

	void makeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
		isEmpty = false;
		busInterfacePassthrough->makeConnection(pauseGuard, connection);
		idsToTrackInputs.insert(connection.destination.gateId);
		idsToTrackOutputs.insert(connection.source.gateId);
		addedConnections.push_back(connection);
	}

	void revert(SimPauseGuard& pauseGuard);

	void pingOutput(SimPauseGuard& pauseGuard, middle_id_t id) {
		if (isReverting) {
			return;
		}
		if (idsToTrackOutputs.contains(id)) {
			revert(pauseGuard);
		}
	}

	void pingInput(SimPauseGuard& pauseGuard, middle_id_t id) {
		if (isReverting) {
			return;
		}
		if (idsToTrackInputs.contains(id)) {
			revert(pauseGuard);
		}
	}

	bool getIsEmpty() const {
		return isEmpty;
	}

	middle_id_t getNewId() {
		middle_id_t newId = middleIdProvider->getNewId();
		reservedIds.push_back(newId);
		return newId;
	}

	void trackOutput(middle_id_t id) {
		idsToTrackOutputs.insert(id);
	}
	void trackInput(middle_id_t id) {
		idsToTrackInputs.insert(id);
	}

private:
	Replacer* replacer;
	BusInterfacePassthrough* busInterfacePassthrough;
	IdProvider<middle_id_t>* middleIdProvider;
	std::unordered_map<middle_id_t, middle_id_t>* replacedIds;
	std::unordered_map<middle_id_t, std::unordered_map<connection_port_id_t, EvalConnectionPoint>>* replacedConnectionPoints;
	std::unordered_set<middle_id_t>* replacementIds;
	std::vector<ReplacementGate> addedGates;
	std::vector<ReplacementGate> deletedGates;
	std::vector<EvalConnection> addedConnections;
	std::vector<EvalConnection> deletedConnections;
	std::vector<middle_id_t> reservedIds;
	std::set<middle_id_t> idsToTrackOutputs;
	std::set<middle_id_t> idsToTrackInputs;
	bool isEmpty { true };
	bool isReverting { false };
};

#endif /* replacement_h */