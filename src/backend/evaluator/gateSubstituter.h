#ifndef gateSubstituter_h
#define gateSubstituter_h

#include "replacer.h"
#include "evalConfig.h"
#include "evalConnection.h"
#include "evalTypedef.h"
#include "idProvider.h"
#include "logicSimulator.h"

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

struct TristateBuffer {
	middle_id_t id;
	middle_id_t junctionId1;
	middle_id_t junctionId2;
};

class GateSubstituter {
public:
	GateSubstituter(
		EvalConfig& evalConfig,
		IdProvider<middle_id_t>& middleIdProvider,
		std::vector<simulator_id_t>& dirtySimulatorIds,
		BlockDataManager& blockDataManager) :
		replacer(evalConfig, middleIdProvider, dirtySimulatorIds, blockDataManager),
		middleIdProvider(middleIdProvider) {}

	void addGate(SimPauseGuard& pauseGuard, const BlockType blockType, const middle_id_t gateId) {
		if (blockType == BlockType::BUTTON || blockType == BlockType::SWITCH || blockType == BlockType::TICK_BUTTON) {
			addTrackedGate({ gateId, blockType, blockType, BlockType::JUNCTION, {}, {}, 1 });
			replacer.addGate(pauseGuard, blockType, gateId);
		} else if (blockType == BlockType::TRISTATE_BUFFER) {
			replacer.addGate(pauseGuard, blockType, gateId);
			addTrackedTristateBuffer(pauseGuard, gateId);
		} else if (blockType == BlockType::LIGHT) {
			replacer.addGate(pauseGuard, BlockType::JUNCTION, gateId);
		} else {
			replacer.addGate(pauseGuard, blockType, gateId);
		}
	}
	void removeGate(SimPauseGuard& pauseGuard, const middle_id_t gateId) {
		replacer.removeGate(pauseGuard, gateId);
		if (isTrackedTristateBuffer(gateId)) {
			deleteTrackedTristateBuffer(pauseGuard, gateId);
		}
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
	inline std::vector<SimulatorStateAndPinSimId> getSimulatorIds(const std::vector<EvalConnectionPoint>& points) const {
		return replacer.getSimulatorIds(points);
	}
	inline std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> getBlockSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
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
		if (isTrackedTristateBuffer(destinationGateId)) {
			// redirect connection to junction
			if (connection.destination.portId == 0) {
				connection.destination.gateId = tristateBuffers.at(destinationGateId).junctionId1;
			} else if (connection.destination.portId == 1) {
				connection.destination.gateId = tristateBuffers.at(destinationGateId).junctionId2;
			} else {
				return;
			}
		}
		if (trackedGates.contains(sourceGateId)) {
			TrackedGate& trackedGate = trackedGates.at(sourceGateId);
			trackedGate.addOutput(connection);
		}
		if (trackedGates.contains(destinationGateId)) {
			TrackedGate& trackedGate = trackedGates.at(destinationGateId);
			trackedGate.addInput(connection);
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
		replacer.makeConnection(pauseGuard, connection);
	}
	void removeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
		replacer.removeConnection(pauseGuard, connection);
		middle_id_t sourceGateId = connection.source.gateId;
		middle_id_t destinationGateId = connection.destination.gateId;
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
	std::unordered_map<middle_id_t, TristateBuffer> tristateBuffers;
	void addTrackedGate(const TrackedGate& gate) {
		trackedGates[gate.id] = gate;
	}
	void deleteTrackedGate(middle_id_t gateId) {
		trackedGates.erase(gateId);
	}
	bool isTrackedGate(middle_id_t gateId) const {
		return trackedGates.contains(gateId);
	}
	void addTrackedTristateBuffer(SimPauseGuard& simPauseGuard, middle_id_t bufferId) {
		middle_id_t junctionId1 = middleIdProvider.getNewId();
		middle_id_t junctionId2 = middleIdProvider.getNewId();
		replacer.addGate(simPauseGuard, BlockType::JUNCTION, junctionId1);
		replacer.addGate(simPauseGuard, BlockType::JUNCTION, junctionId2);
		tristateBuffers[bufferId] = { bufferId, junctionId1, junctionId2 };
		replacer.makeConnection(simPauseGuard, EvalConnection(EvalConnectionPoint(junctionId1, 0), EvalConnectionPoint(bufferId, 0)));
		replacer.makeConnection(simPauseGuard, EvalConnection(EvalConnectionPoint(junctionId2, 0), EvalConnectionPoint(bufferId, 1)));
	}
	void deleteTrackedTristateBuffer(SimPauseGuard& simPauseGuard, middle_id_t bufferId) {
		replacer.removeGate(simPauseGuard, tristateBuffers.at(bufferId).junctionId1);
		replacer.removeGate(simPauseGuard, tristateBuffers.at(bufferId).junctionId2);
		middleIdProvider.releaseId(tristateBuffers.at(bufferId).junctionId1);
		middleIdProvider.releaseId(tristateBuffers.at(bufferId).junctionId2);
		tristateBuffers.erase(bufferId);
	}
	bool isTrackedTristateBuffer(middle_id_t bufferId) const {
		return tristateBuffers.contains(bufferId);
	}
};

#endif /* gateSubstituter_h */
