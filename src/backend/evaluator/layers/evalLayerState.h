#ifndef evalLayerState_h
#define evalLayerState_h

#include "../evalDefs.h"
#include "backend/blockData/blockDataManager.h"

struct EvalGate {
	EvalGate(EvalGateType type, eval_gate_id gateId) : type(type), gateId(gateId) { }

	EvalGateType type;
	eval_gate_id gateId;

	std::unordered_map<connection_end_id_t, std::unordered_set<EvalConnectionPoint>> connections;
};

class BlockDataManager;

class EvalLayerState {
public:
	EvalLayerState(const BlockDataManager& blockDataManager, unsigned int layerIndex = 0) :
		blockDataManager(blockDataManager), layerIndex(layerIndex),
		lastUnsedEvalGateId(std::numeric_limits<eval_gate_id::rep>::max() - 1 - layerIndex * 10000000) {}

	void addGate(eval_gate_id gateId, EvalGateType type);
	void removeGate(eval_gate_id gateId);
	void addConnection(const EvalConnection& evalConnection, unsigned int weight = 1);
	void removeConnection(const EvalConnection& evalConnection, unsigned int weight = 1);

 	const EvalGate* getGate(eval_gate_id gateId) const {
		auto iter = gates.find(gateId);
		if (iter == gates.end()) return nullptr;
		return &iter->second;
	}

	std::unordered_set<eval_gate_id>::const_iterator getAddedGatesBegin() const { return addedGates.begin(); }
	std::unordered_set<eval_gate_id>::const_iterator getAddedGatesEnd() const { return addedGates.end(); }
	bool addEditContainsGate(eval_gate_id evalGateId) const { return addedGates.contains(evalGateId); }

	std::unordered_set<eval_gate_id>::const_iterator getRemovedGatesBegin() const { return removedGates.begin(); }
	std::unordered_set<eval_gate_id>::const_iterator getRemovedGatesEnd() const { return removedGates.end(); }
	bool removeEditContainsGate(eval_gate_id evalGateId) const { return removedGates.contains(evalGateId); }

	std::unordered_map<EvalConnection, unsigned int>::const_iterator getAddedConnectionsBegin() const { return addedConnections.begin(); }
	std::unordered_map<EvalConnection, unsigned int>::const_iterator getAddedConnectionsEnd() const { return addedConnections.end(); }
	bool addEditContainsConnection(EvalConnection evalConnection) const { return addedConnections.contains(evalConnection); }

	std::unordered_map<EvalConnection, unsigned int>::const_iterator getRemovedConnectionsBegin() const { return removedConnections.begin(); }
	std::unordered_map<EvalConnection, unsigned int>::const_iterator getRemovedConnectionsEnd() const { return removedConnections.end(); }
	bool removeEditContainsConnection(EvalConnection evalConnection) const { return removedConnections.contains(evalConnection); }

	unsigned int getConnectionWeight(EvalConnection connection) const {
		auto iter = connectionWeights.find(connection);
		if (iter == connectionWeights.end()) return 1;
		return iter->second;
	}
	void setConnectionWeight(EvalConnection connection, unsigned int weight) {
		assert(weight > 1);
		connectionWeights.insert_or_assign(connection, weight);
	}
	void addConnectionWeight(EvalConnection connection, int weightChange) {
		assert(weightChange != 0); // just dont
		auto iter = connectionWeights.find(connection);
		if (iter == connectionWeights.end()) {
			if (weightChange == 1) return;
			connectionWeights.emplace(connection, weightChange);
		}
		iter->second += weightChange;
		if (iter->second == 1) {
			connectionWeights.erase(iter);
		}
		assert(iter->second != 0);
	}
	void removeConnectionWeight(EvalConnection connection) {
		unsigned int deletedCount = connectionWeights.erase(connection);
		assert(deletedCount == 1);
	}

	void resetEdits() {
		// lastUnsedEvalGateId = std::numeric_limits<eval_gate_id::rep>::max() - 1; // -1 in case of math errors with this high number
		addedGates.clear();
		removedGates.clear();
		addedConnections.clear();
		removedConnections.clear();
	}

	EvalLayerState& getOrMakeNextLayerState() {
		if (!nextLayerState) {
			nextLayerState = std::make_unique<EvalLayerState>(blockDataManager, layerIndex + 1);
			nextLayerState->setLastLayer(this);
		}
		return *nextLayerState;
	}
	EvalLayerState* getNextLayerState() {
		return nextLayerState.get();
	}
	const EvalLayerState* getNextLayerState() const {
		return nextLayerState.get();
	}
	const EvalLayerState* getLastLayerState() const {
		return lastLayerState;
	}
	std::unordered_map<eval_gate_id, eval_gate_id>& getGateIdRemapping() { return gateIdRemapping; }
	const std::unordered_map<eval_gate_id, eval_gate_id>& getGateIdRemapping() const { return gateIdRemapping; }
	std::unordered_multimap<eval_gate_id, eval_gate_id>& getGateIdReverseRemapping() { return gateIdReverseRemapping; }
	const std::unordered_multimap<eval_gate_id, eval_gate_id>& getGateIdReverseRemapping() const { return gateIdReverseRemapping; }
	std::unordered_map<EvalConnectionPoint, EvalConnectionPoint>& getConnectionPointRemapping() { return connectionPointRemapping; }
	const std::unordered_map<EvalConnectionPoint, EvalConnectionPoint>& getConnectionPointRemapping() const { return connectionPointRemapping; }
	std::unordered_multimap<EvalConnectionPoint, EvalConnectionPoint>& getConnectionPointReverseRemapping() { return connectionPointReverseRemapping; }
	const std::unordered_multimap<EvalConnectionPoint, EvalConnectionPoint>& getConnectionPointReverseRemapping() const { return connectionPointReverseRemapping; }

	eval_gate_id getUnsedEvalGateId() {
		do lastUnsedEvalGateId = lastUnsedEvalGateId.get() -1;
		while (gates.contains(lastUnsedEvalGateId));
		return lastUnsedEvalGateId;
	}

	void visualize() const;

private:
	eval_gate_id lastUnsedEvalGateId; // -1 in case of math errors with this high number

	void setLastLayer(const EvalLayerState* lastLayerState) { this->lastLayerState = lastLayerState; }

	std::unique_ptr<EvalLayerState> nextLayerState;
	const EvalLayerState* lastLayerState;
	unsigned int layerIndex;

	const BlockDataManager& blockDataManager;

	std::unordered_map<eval_gate_id, EvalGate> gates;
	std::unordered_map<EvalConnection, unsigned int> connectionWeights;

	// all remappings will be eval_gate_id or EvalConnectionPoint
	std::unordered_map<eval_gate_id, eval_gate_id> gateIdRemapping;
	std::unordered_multimap<eval_gate_id, eval_gate_id> gateIdReverseRemapping;
	std::unordered_map<EvalConnectionPoint, EvalConnectionPoint> connectionPointRemapping;
	std::unordered_multimap<EvalConnectionPoint, EvalConnectionPoint> connectionPointReverseRemapping;

	std::unordered_set<eval_gate_id> addedGates;
	std::unordered_set<eval_gate_id> removedGates;
	std::unordered_map<EvalConnection, unsigned int> addedConnections;
	std::unordered_map<EvalConnection, unsigned int> removedConnections;
};

#endif /* evalLayerState_h */
