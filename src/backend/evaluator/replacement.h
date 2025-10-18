#ifndef replacement_h
#define replacement_h

#include "evalConnection.h"
#include "evalDefs.h"
#include "backend/container/block/blockDefs.h"

class SimPauseGuard;
class Replacer;

struct ReplacementGate {
	middle_id_t id;
	BlockType type;
};

class Replacement {
public:
	Replacement(
		Replacer* replacer,
		int layer
		) :
		replacer(replacer),
		layer(layer) {}

	void removeGate(SimPauseGuard& pauseGuard, middle_id_t gateId, std::unordered_map<connection_port_id_t, EvalConnectionPoint> replacementConnectionPoints);

	void removeGate(SimPauseGuard& pauseGuard, middle_id_t gateId, middle_id_t replacementId);

	void addGate(SimPauseGuard& pauseGuard, BlockType blockType, middle_id_t gateId);

	void removeConnection(SimPauseGuard& pauseGuard, EvalConnection connection);

	void makeConnection(SimPauseGuard& pauseGuard, EvalConnection connection);

	void overrideConnectionPoint(EvalConnectionPoint originalPoint, EvalConnectionPoint replacementPoint);

	void markIdAsReplaced(middle_id_t originalId, int overpowerLayer);

	void revert(SimPauseGuard& pauseGuard);

	void addRevertAction(std::function<void(SimPauseGuard&)> callback) {
		isEmpty = false;
		revertCallbacksWithPauseGuard.push_back(std::move(callback));
	}
	void addRevertAction(std::function<void()> callback) {
		isEmpty = false;
		revertCallbacks.push_back(std::move(callback));
	}

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

	middle_id_t getNewId();

	void trackOutput(middle_id_t id) {
		idsToTrackOutputs.insert(id);
	}
	void trackInput(middle_id_t id) {
		idsToTrackInputs.insert(id);
	}
	void trackConnection(EvalConnection conn) {
		trackOutput(conn.source.gateId);
		trackInput(conn.destination.gateId);
	}
	void trackGate(middle_id_t id) {
		trackOutput(id);
		trackInput(id);
	}

private:
	Replacer* replacer;
	std::vector<ReplacementGate> addedGates;
	std::vector<ReplacementGate> deletedGates;
	std::vector<EvalConnection> addedConnections;
	std::vector<EvalConnection> deletedConnections;
	std::vector<middle_id_t> reservedIds;
	std::set<middle_id_t> idsToTrackOutputs;
	std::set<middle_id_t> idsToTrackInputs;
	std::vector<std::function<void(SimPauseGuard&)>> revertCallbacksWithPauseGuard;
	std::vector<std::function<void()>> revertCallbacks;
	bool isEmpty { true };
	bool isReverting { false };
	int layer { 0 };
};

#endif /* replacement_h */
