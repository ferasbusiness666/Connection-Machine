#ifndef gateSubstituter_h
#define gateSubstituter_h

#include "evalConfig.h"
#include "evalConnection.h"
#include "evalDefs.h"
#include "logicSimulator.h"
#include "replacer.h"

struct TrackedGate {
	middle_id_t id;
	BlockType currentState;
	BlockType falseState;
	BlockType trueState;
	std::vector<EvalConnection> inputs;
	std::vector<EvalConnection> outputs;
	unsigned int numInputsForTrue;

	BlockType evaluate() {
		if (inputs.size() >= numInputsForTrue) {
			return trueState;
		} else {
			return falseState;
		}
	}

	void addInput(EvalConnection connection) {
		inputs.push_back(connection);
	}

	void addOutput(EvalConnection connection) {
		outputs.push_back(connection);
	}

	void removeInput(EvalConnection connection) {
		auto it = std::find_if(inputs.begin(), inputs.end(),
			[&](const EvalConnection& conn) { return conn.source == connection.source && conn.destination == connection.destination; });
		if (it != inputs.end()) {
			inputs.erase(it);
		}
	}

	void removeOutput(EvalConnection connection) {
		auto it = std::find_if(outputs.begin(), outputs.end(),
			[&](const EvalConnection& conn) { return conn.source == connection.source && conn.destination == connection.destination; });
		if (it != outputs.end()) {
			outputs.erase(it);
		}
	}

	bool removeReferencesToId(const middle_id_t id) {
		size_t initialSize = inputs.size() + outputs.size();
		inputs.erase(std::remove_if(inputs.begin(), inputs.end(),
			[&](const EvalConnection& conn) { return conn.source.gateId == id || conn.destination.gateId == id; }), inputs.end());
		outputs.erase(std::remove_if(outputs.begin(), outputs.end(),
			[&](const EvalConnection& conn) { return conn.source.gateId == id || conn.destination.gateId == id; }), outputs.end());
		return (initialSize != inputs.size() + outputs.size());
	}
};

struct GateWithLinkedInputs {
	middle_id_t id;
	std::unordered_map<connection_port_id_t, middle_id_t> linkedInputs; // portId -> junctionId
};

class GateSubstituter {
public:
	GateSubstituter(
		EvalConfig& evalConfig,
		IdProvider<middle_id_t>& middleIdProvider,
		std::vector<simulator_id_t>& dirtySimulatorIds,
		std::vector<middle_id_t>& dirtyMiddleIds,
		BlockDataManager& blockDataManager) :
		replacer(evalConfig, middleIdProvider, dirtySimulatorIds, dirtyMiddleIds, blockDataManager),
		middleIdProvider(middleIdProvider) {}

	void addGate(SimPauseGuard& pauseGuard, const BlockType blockType, const middle_id_t gateId) {
		if (tryAddGateWithLinkedInputs(pauseGuard, blockType, gateId)) return;
		if (blockType == BlockType::BUTTON || blockType == BlockType::SWITCH || blockType == BlockType::TICK_BUTTON) {
			addTrackedGate({ gateId, blockType, blockType, BlockType::JUNCTION, {}, {}, 1 });
			replacer.addGate(pauseGuard, blockType, gateId);
		} else if (blockType == BlockType::LIGHT) {
			replacer.addGate(pauseGuard, BlockType::JUNCTION, gateId);
		} else {
			replacer.addGate(pauseGuard, blockType, gateId);
		}
	}
	void removeGate(SimPauseGuard& pauseGuard, const middle_id_t gateId) {
		if (isGateWithLinkedInputs(gateId)) {
			deleteGateWithLinkedInputs(pauseGuard, gateId);
		}
		replacer.removeGate(pauseGuard, gateId);
		deleteTrackedGate(gateId);
		for (auto& trackedGate : trackedGates) {
			bool success = trackedGate.second.removeReferencesToId(gateId);
			if (success) {
				TrackedGate& trackedGateRef = trackedGate.second;
				BlockType newState = trackedGateRef.evaluate();
				if (newState != trackedGateRef.currentState) {
					trackedGateRef.currentState = newState;
					replacer.removeGate(pauseGuard, trackedGateRef.id);
					replacer.addGate(pauseGuard, newState, trackedGateRef.id);
					for (const auto& input : trackedGateRef.inputs) {
						replacer.makeConnection(pauseGuard, input);
					}
					for (const auto& output : trackedGateRef.outputs) {
						if (output.source.gateId == output.destination.gateId) {
							continue;
						}
						replacer.makeConnection(pauseGuard, output);
					}
				}
			}
		}
	}
	inline SimPauseGuard beginEdit() {
		return replacer.beginEdit();
	}
	inline void endEdit(SimPauseGuard& pauseGuard) {
		replacer.endEdit(pauseGuard);
	}

	inline logic_state_t getState(EvalConnectionPoint point) const {
		return replacer.getState(point);
	}
	inline std::vector<logic_state_t> getStates(const std::vector<EvalConnectionPoint>& points) const {
		return replacer.getStates(points);
	}
	inline std::vector<logic_state_t> getPinStates(const std::vector<EvalConnectionPoint>& points) const {
		return replacer.getPinStates(points);
	}
	inline std::vector<logic_state_t> getStatesFromSimulatorIds(const std::vector<simulator_id_t>& simulatorIds) const {
		return replacer.getStatesFromSimulatorIds(simulatorIds);
	}
	inline std::vector<simulator_id_t> getBlockSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
		return replacer.getBlockSimulatorIds(points);
	}
	inline std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> getPinSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
		return replacer.getPinSimulatorIds(points);
	}
	inline void setState(EvalConnectionPoint point, logic_state_t state) {
		replacer.setState(point, state);
	}
	void makeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
		middle_id_t sourceGateId = connection.source.gateId;
		middle_id_t destinationGateId = connection.destination.gateId;
		redirectConnectionToLinkedInput(connection, destinationGateId);
		if (trackedGates.contains(sourceGateId)) {
			TrackedGate& trackedGate = trackedGates.at(sourceGateId);
			trackedGate.addOutput(connection);
		}
		if (trackedGates.contains(destinationGateId)) {
			TrackedGate& trackedGate = trackedGates.at(destinationGateId);
			trackedGate.addInput(connection);
			BlockType newState = trackedGate.evaluate();
			if (newState == trackedGate.currentState) {
				replacer.makeConnection(pauseGuard, connection);
				return;
			}
			trackedGate.currentState = newState;
			replacer.removeGate(pauseGuard, destinationGateId);
			replacer.addGate(pauseGuard, newState, destinationGateId);
			for (const auto& input : trackedGate.inputs) {
				replacer.makeConnection(pauseGuard, input);
			}
			for (const auto& output : trackedGate.outputs) {
				if (output.source.gateId == output.destination.gateId) {
					continue;
				}
				replacer.makeConnection(pauseGuard, output);
			}
			return;
		}
		replacer.makeConnection(pauseGuard, connection);
	}
	void removeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
		middle_id_t sourceGateId = connection.source.gateId;
		middle_id_t destinationGateId = connection.destination.gateId;
		redirectConnectionToLinkedInput(connection, destinationGateId);
		replacer.removeConnection(pauseGuard, connection);
		if (trackedGates.contains(sourceGateId)) {
			TrackedGate& trackedGate = trackedGates.at(sourceGateId);
			trackedGate.removeOutput(connection);
		}
		if (trackedGates.contains(destinationGateId)) {
			TrackedGate& trackedGate = trackedGates.at(destinationGateId);
			trackedGate.removeInput(connection);
			BlockType newState = trackedGate.evaluate();
			if (newState != trackedGate.currentState) {
				trackedGate.currentState = newState;
				replacer.removeGate(pauseGuard, destinationGateId);
				replacer.addGate(pauseGuard, newState, destinationGateId);
				for (const auto& input : trackedGate.inputs) {
					replacer.makeConnection(pauseGuard, input);
				}
				for (const auto& output : trackedGate.outputs) {
					if (output.source.gateId == output.destination.gateId) {
						continue;
					}
					replacer.makeConnection(pauseGuard, output);
				}
			}
		}
	}

	inline double getAverageTickrate() const {
		return replacer.getAverageTickrate();
	}

private:
	Replacer replacer;
	IdProvider<middle_id_t>& middleIdProvider;
	std::unordered_map<middle_id_t, TrackedGate> trackedGates;
	std::unordered_map<middle_id_t, GateWithLinkedInputs> gatesWithLinkedInputs;
	std::unordered_map<middle_id_t, std::pair<middle_id_t, connection_port_id_t>> linkedInputJunctionLookup;
	void addTrackedGate(const TrackedGate& gate) {
		trackedGates[gate.id] = gate;
	}
	void deleteTrackedGate(middle_id_t gateId) {
		trackedGates.erase(gateId);
	}
	bool isTrackedGate(middle_id_t gateId) const {
		return trackedGates.contains(gateId);
	}
	bool tryAddGateWithLinkedInputs(SimPauseGuard& pauseGuard, BlockType blockType, middle_id_t gateId) {
		auto configIter = blockTypesWithLinkedInputs.find(blockType);
		if (configIter == blockTypesWithLinkedInputs.end()) {
			return false;
		}
		replacer.addGate(pauseGuard, blockType, gateId);
		GateWithLinkedInputs gateWithLinkedInputs{ gateId, {} };
		for (connection_port_id_t portId : configIter->second) {
			middle_id_t junctionId = middleIdProvider.getNewId();
			replacer.addGate(pauseGuard, BlockType::JUNCTION, junctionId);
			replacer.makeConnection(pauseGuard, EvalConnection(EvalConnectionPoint(junctionId, 0), EvalConnectionPoint(gateId, portId)));
			gateWithLinkedInputs.linkedInputs[portId] = junctionId;
			linkedInputJunctionLookup[junctionId] = { gateId, portId };
		}
		gatesWithLinkedInputs[gateId] = std::move(gateWithLinkedInputs);
		return true;
	}
	void deleteGateWithLinkedInputs(SimPauseGuard& pauseGuard, middle_id_t gateId) {
		if (!isGateWithLinkedInputs(gateId)) {
			return;
		}
		GateWithLinkedInputs& gate = gatesWithLinkedInputs.at(gateId);
		for (const auto& [portId, junctionId] : gate.linkedInputs) {
			replacer.removeConnection(pauseGuard, EvalConnection(EvalConnectionPoint(junctionId, 0), EvalConnectionPoint(gateId, portId)));
			replacer.removeGate(pauseGuard, junctionId);
			middleIdProvider.releaseId(junctionId);
			linkedInputJunctionLookup.erase(junctionId);
		}
		gatesWithLinkedInputs.erase(gateId);
	}
	bool isGateWithLinkedInputs(middle_id_t gateId) const {
		return gatesWithLinkedInputs.contains(gateId);
	}
	bool redirectConnectionToLinkedInput(EvalConnection& connection, middle_id_t& originalGateId) {
		auto gateIter = gatesWithLinkedInputs.find(connection.destination.gateId);
		if (gateIter == gatesWithLinkedInputs.end()) {
			return false;
		}
		auto linkIter = gateIter->second.linkedInputs.find(connection.destination.portId);
		if (linkIter == gateIter->second.linkedInputs.end()) {
			return false;
		}
		originalGateId = gateIter->first;
		connection.destination.gateId = linkIter->second;
		return true;
	}

	std::map<BlockType, std::vector<connection_port_id_t>> blockTypesWithLinkedInputs = {
		{BlockType::TRISTATE_BUFFER, {0, 1}},
		{BlockType::NOT, {0}},
		{BlockType::BUFFER, {0}}
	};
};

#endif /* gateSubstituter_h */
