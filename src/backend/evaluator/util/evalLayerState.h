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
	EvalLayerState(const BlockDataManager& blockDataManager) : blockDataManager(blockDataManager) {}

	// will also update connection points accordingly
	void passAddGate(eval_gate_id gateId) {
		assert(lastLayerState);
		const EvalGate* evalGate = lastLayerState->getGate(gateId);
		const BlockData* blockData = blockDataManager.getBlockData(getBlockType(evalGate->type));
		assert(blockData);
		for (auto iter : blockData->getConnectionsSafe()) {
			connectionPointRemapping.emplace(EvalConnectionPoint(gateId, iter.first), EvalConnectionPoint(gateId, iter.first));
			connectionPointReverseRemapping.emplace(EvalConnectionPoint(gateId, iter.first), EvalConnectionPoint(gateId, iter.first));
		}
		addGate(gateId, evalGate->type);
	}
	void passRemoveGate(eval_gate_id gateId) {
		assert(lastLayerState);
		const EvalGate* evalGate = getGate(gateId);
		const BlockData* blockData = blockDataManager.getBlockData(getBlockType(evalGate->type));
		assert(blockData);
		for (auto iter : blockData->getConnectionsSafe()) {
			connectionPointRemapping.erase(EvalConnectionPoint(gateId, iter.first));
			connectionPointReverseRemapping.erase(EvalConnectionPoint(gateId, iter.first));
		}
		removeGate(gateId);
	}
	void passAddConnection(const EvalConnection& evalConnection) {
		assert(lastLayerState);
		addConnection(evalConnection);
	}
	void passRemoveConnection(const EvalConnection& evalConnection) {
		assert(lastLayerState);
		removeConnection(evalConnection);
	}
	void addGate(eval_gate_id gateId, EvalGateType type) { // TODO: add transparent junction merging
		bool suc = gates.try_emplace(gateId, type, gateId).second;
		assert(suc);
		addedGates.insert(gateId);
		assert(!removedGates.contains(gateId));
	}
	void removeGate(eval_gate_id gateId) { // TODO: add transparent junction splitting
		auto iter = gates.find(gateId);
		assert(iter != gates.end());
		assert(iter->second.connections.empty());
		gates.erase(iter);
		if (addedGates.erase(gateId) == 0) {
			removedGates.insert(gateId);
		}
	}
	void addConnection(const EvalConnection& evalConnection) { // TODO: add transparent junction merging
		auto gateAIterBoolPair = gates.find(evalConnection.connectionPointA.gateId);
		assert(gateAIterBoolPair != gates.end());
		bool suc = gateAIterBoolPair->second.connections[evalConnection.connectionPointA.connectionEndId].insert(evalConnection.connectionPointB).second;
		assert(suc);

		auto gateBIterBoolPair = gates.find(evalConnection.connectionPointB.gateId);
		assert(gateBIterBoolPair != gates.end());
		gateBIterBoolPair->second.connections[evalConnection.connectionPointB.connectionEndId].insert(evalConnection.connectionPointA);
		addedConnections.insert(evalConnection);
		assert(!removedConnections.contains(evalConnection));
	}
	void removeConnection(const EvalConnection& evalConnection) { // TODO: add transparent junction splitting
		auto gateAIterBoolPair = gates.find(evalConnection.connectionPointA.gateId);
		auto gateAConnectionIter = gateAIterBoolPair->second.connections.find(evalConnection.connectionPointA.connectionEndId);
		if (gateAConnectionIter->second.size() == 1) gateAIterBoolPair->second.connections.erase(gateAConnectionIter);
		else {
			bool suc = gateAConnectionIter->second.erase(evalConnection.connectionPointB);
			assert(suc);
		}

		auto gateBIterBoolPair = gates.find(evalConnection.connectionPointB.gateId);
		auto gateBConnectionIter = gateBIterBoolPair->second.connections.find(evalConnection.connectionPointB.connectionEndId);
		if (gateBConnectionIter->second.size() == 1) gateBIterBoolPair->second.connections.erase(gateBConnectionIter);
		else gateBConnectionIter->second.erase(evalConnection.connectionPointA);
		if (addedConnections.erase(evalConnection) == 0) {
			removedConnections.insert(evalConnection);
		}
	}

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

	std::unordered_set<EvalConnection>::const_iterator getAddedConnectionsBegin() const { return addedConnections.begin(); }
	std::unordered_set<EvalConnection>::const_iterator getAddedConnectionsEnd() const { return addedConnections.end(); }
	bool addEditContainsConnection(EvalConnection evalConnection) const { return addedConnections.contains(evalConnection); }

	std::unordered_set<EvalConnection>::const_iterator getRemovedConnectionsBegin() const { return removedConnections.begin(); }
	std::unordered_set<EvalConnection>::const_iterator getRemovedConnectionsEnd() const { return removedConnections.end(); }
	bool removeEditContainsConnection(EvalConnection evalConnection) const { return removedConnections.contains(evalConnection); }

	void resetEdits() {
		addedGates.clear();
		removedGates.clear();
		addedConnections.clear();
		removedConnections.clear();
	}

	EvalLayerState& getNextLayerState() {
		if (!nextLayerState) {
			nextLayerState = std::make_unique<EvalLayerState>(blockDataManager);
			nextLayerState->setLastLayer(this);
		}
		return *nextLayerState;
	}
	const EvalLayerState* getNextLayerState() const {
		return nextLayerState.get();
	}
	const std::unordered_map<EvalConnectionPoint, EvalConnectionPoint>& getConnectionPointRemapping() const { return connectionPointRemapping; }
	const std::unordered_multimap<EvalConnectionPoint, EvalConnectionPoint>& getConnectionPointReverseRemapping() const { return connectionPointReverseRemapping; }

private:
	void setLastLayer(const EvalLayerState* lastLayerState) { this->lastLayerState = lastLayerState; }

	std::unique_ptr<EvalLayerState> nextLayerState;
	const EvalLayerState* lastLayerState;

	const BlockDataManager& blockDataManager;

	std::unordered_map<eval_gate_id, EvalGate> gates;
	std::unordered_map<EvalConnectionPoint, EvalConnectionPoint> connectionPointRemapping;
	std::unordered_multimap<EvalConnectionPoint, EvalConnectionPoint> connectionPointReverseRemapping;

	std::unordered_set<eval_gate_id> addedGates;
	std::unordered_set<eval_gate_id> removedGates;
	std::unordered_set<EvalConnection> addedConnections;
	std::unordered_set<EvalConnection> removedConnections;
};

#endif /* evalLayerState_h */
