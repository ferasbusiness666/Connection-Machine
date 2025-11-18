#ifndef gateSubstituter_h
#define gateSubstituter_h

#include "backend/evaluator/util/evalConnection.h"
#include "backend/evaluator/util/evalConfig.h"
#include "backend/evaluator/evalDefs.h"
#include "layer4_replacer.h"

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

	bool removeInput(EvalConnection connection) {
		auto it = std::find_if(inputs.begin(), inputs.end(),
			[&](const EvalConnection& conn) { return conn.source == connection.source && conn.destination == connection.destination; });
		if (it != inputs.end()) {
			inputs.erase(it);
			return true;
		}
		return false;
	}

	bool removeOutput(EvalConnection connection) {
		auto it = std::find_if(outputs.begin(), outputs.end(),
			[&](const EvalConnection& conn) { return conn.source == connection.source && conn.destination == connection.destination; });
		if (it != outputs.end()) {
			outputs.erase(it);
			return true;
		}
		return false;
	}

	bool removeReferencesToId(const middle_id_t id) {
#ifdef TRACY_PROFILER
		ZoneScopedN("TrackedGate::removeReferencesToId");
#endif
		size_t initialSize = inputs.size() + outputs.size();
		inputs.erase(std::remove_if(inputs.begin(), inputs.end(),
			[&](const EvalConnection& conn) { return conn.source.gateId == id || conn.destination.gateId == id; }), inputs.end());
		outputs.erase(std::remove_if(outputs.begin(), outputs.end(),
			[&](const EvalConnection& conn) { return conn.source.gateId == id || conn.destination.gateId == id; }), outputs.end());
		return (initialSize != inputs.size() + outputs.size());
	}

	nlohmann::json dumpState() const {
		nlohmann::json stateJson;
		stateJson["id"] = id.get();
		stateJson["currentState"] = blocktype_to_string(currentState);
		stateJson["falseState"] = blocktype_to_string(falseState);
		stateJson["trueState"] = blocktype_to_string(trueState);
		stateJson["numInputsForTrue"] = numInputsForTrue;
		stateJson["inputs"] = nlohmann::json::array();
		for (const auto& input : inputs) {
			stateJson["inputs"].push_back(input.dumpState());
		}
		stateJson["outputs"] = nlohmann::json::array();
		for (const auto& output : outputs) {
			stateJson["outputs"].push_back(output.dumpState());
		}
		return stateJson;
	}
};

struct GateWithLinkedIO {
	middle_id_t id;
	std::vector<middle_id_t> idsCreated;
	std::unordered_map<connection_end_id_t, EvalConnectionPoint> linkedIO;
	nlohmann::json dumpState() const {
		nlohmann::json stateJson;
		stateJson["id"] = id.get();
		stateJson["idsCreated"] = nlohmann::json::array();
		for (const auto& createdId : idsCreated) {
			stateJson["idsCreated"].push_back(createdId.get());
		}
		stateJson["linkedIO"] = nlohmann::json::object();
		for (const auto& [portId, point] : linkedIO) {
			stateJson["linkedIO"][std::to_string(portId.get())] = point.dumpState();
		}
		return stateJson;
	}
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
		middleIdProvider(middleIdProvider),
		blockDataManager(blockDataManager) {}

	void addGate(SimPauseGuard& pauseGuard, const BlockType blockType, const middle_id_t gateId) {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		if (tryAddGateWithLinkedIO(pauseGuard, blockType, gateId)) return;
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
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		if (gatesWithLinkedIO.contains(gateId)) {
			deleteGateWithLinkedIO(pauseGuard, gateId);
		}
		replacer.removeGate(pauseGuard, gateId);
		std::vector<middle_id_t> referencingGates = collectReferencingTrackedGates(gateId);
		deleteTrackedGate(gateId);
		for (const auto& referencingGateId : referencingGates) {
#ifdef TRACY_PROFILER
			ZoneScopedN("GateSubstituter::removeGate - per tracked gate");
#endif
			auto trackedGateIter = trackedGates.find(referencingGateId);
			if (trackedGateIter == trackedGates.end()) {
				continue;
			}
			TrackedGate& trackedGateRef = trackedGateIter->second;
			bool success = trackedGateRef.removeReferencesToId(gateId);
			if (success) {
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
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		middle_id_t sourceGateId = connection.source.gateId;
		middle_id_t destinationGateId = connection.destination.gateId;
		redirectConnectionToLinked(connection);
		if (trackedGates.contains(sourceGateId)) {
			TrackedGate& trackedGate = trackedGates.at(sourceGateId);
			trackedGate.addOutput(connection);
			registerTrackedGateReference(connection.destination.gateId, sourceGateId);
		}
		if (trackedGates.contains(destinationGateId)) {
			TrackedGate& trackedGate = trackedGates.at(destinationGateId);
			trackedGate.addInput(connection);
			registerTrackedGateReference(connection.source.gateId, destinationGateId);
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
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		middle_id_t sourceGateId = connection.source.gateId;
		middle_id_t destinationGateId = connection.destination.gateId;
		redirectConnectionToLinked(connection);
		replacer.removeConnection(pauseGuard, connection);
		if (trackedGates.contains(sourceGateId)) {
			TrackedGate& trackedGate = trackedGates.at(sourceGateId);
			bool removed = trackedGate.removeOutput(connection);
			if (removed) {
				unregisterTrackedGateReference(connection.destination.gateId, sourceGateId);
			}
		}
		if (trackedGates.contains(destinationGateId)) {
			TrackedGate& trackedGate = trackedGates.at(destinationGateId);
			bool removed = trackedGate.removeInput(connection);
			if (removed) {
				unregisterTrackedGateReference(connection.source.gateId, destinationGateId);
			}
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
	inline bool stepBack() {
		return replacer.stepBack();
	}
	inline bool stepForward() {
		return replacer.stepForward();
	}
	inline bool skipBack() {
		return replacer.skipBack();
	}
	inline bool skipForward() {
		return replacer.skipForward();
	}
	inline bool isViewingReplay() const {
		return replacer.isViewingReplay();
	}

	nlohmann::json dumpState() const {
		nlohmann::json stateJson;
		stateJson["replacer"] = replacer.dumpState();
		stateJson["trackedGates"] = nlohmann::json::object();
		for (const auto& [gateId, trackedGate] : trackedGates) {
			stateJson["trackedGates"][std::to_string(gateId.get())] = trackedGate.dumpState();
		}
		stateJson["gatesWithLinkedIO"] = nlohmann::json::object();
		for (const auto& [gateId, gateWithLinkedIO] : gatesWithLinkedIO) {
			stateJson["gatesWithLinkedIO"][std::to_string(gateId.get())] = gateWithLinkedIO.dumpState();
		}
		return stateJson;
	}

private:
	Replacer replacer;
	IdProvider<middle_id_t>& middleIdProvider;
	BlockDataManager& blockDataManager;
	std::unordered_map<middle_id_t, TrackedGate> trackedGates;
	std::unordered_map<middle_id_t, std::unordered_map<middle_id_t, size_t>> gateReferenceIndex;
	std::unordered_map<middle_id_t, GateWithLinkedIO> gatesWithLinkedIO;
	void addTrackedGate(const TrackedGate& gate) {
		trackedGates[gate.id] = gate;
		for (const auto& input : gate.inputs) {
			registerTrackedGateReference(input.source.gateId, gate.id);
		}
		for (const auto& output : gate.outputs) {
			registerTrackedGateReference(output.destination.gateId, gate.id);
		}
	}
	void deleteTrackedGate(middle_id_t gateId) {
		auto trackedGateIter = trackedGates.find(gateId);
		if (trackedGateIter == trackedGates.end()) {
			return;
		}
		removeAllReferencesForTrackedGate(trackedGateIter->second);
		trackedGates.erase(trackedGateIter);
	}
	bool isTrackedGate(middle_id_t gateId) const {
		return trackedGates.contains(gateId);
	}
	void registerTrackedGateReference(middle_id_t referencedGateId, middle_id_t trackingGateId) {
		++gateReferenceIndex[referencedGateId][trackingGateId];
	}
	void unregisterTrackedGateReference(middle_id_t referencedGateId, middle_id_t trackingGateId) {
		auto referencedIter = gateReferenceIndex.find(referencedGateId);
		if (referencedIter == gateReferenceIndex.end()) {
			return;
		}
		auto& counts = referencedIter->second;
		auto trackingIter = counts.find(trackingGateId);
		if (trackingIter == counts.end()) {
			return;
		}
		if (--trackingIter->second == 0) {
			counts.erase(trackingIter);
			if (counts.empty()) {
				gateReferenceIndex.erase(referencedIter);
			}
		}
	}
	void removeAllReferencesForTrackedGate(const TrackedGate& gate) {
		for (const auto& input : gate.inputs) {
			unregisterTrackedGateReference(input.source.gateId, gate.id);
		}
		for (const auto& output : gate.outputs) {
			unregisterTrackedGateReference(output.destination.gateId, gate.id);
		}
	}
	std::vector<middle_id_t> collectReferencingTrackedGates(middle_id_t gateId) {
		std::vector<middle_id_t> referencingGates;
		auto referenceIter = gateReferenceIndex.find(gateId);
		if (referenceIter == gateReferenceIndex.end()) {
			return referencingGates;
		}
		referencingGates.reserve(referenceIter->second.size());
		for (const auto& [trackingGateId, _] : referenceIter->second) {
			referencingGates.push_back(trackingGateId);
		}
		gateReferenceIndex.erase(referenceIter);
		return referencingGates;
	}
	bool tryAddGateWithLinkedIO(SimPauseGuard& pauseGuard, BlockType blockType, middle_id_t gateId) {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		if (blockType == BlockType::COLOR_LIGHT) { // this will be expanded later to be dynamic/automatic for all blocks that have non 1-bit inputs
			replacer.addGate(pauseGuard, blockType, gateId);
			middle_id_t busId = middleIdProvider.getNewId();
			replacer.addGate(pauseGuard, blockDataManager.getBusBlock(6), busId);
			GateWithLinkedIO gateWithLinkedIO { gateId, { busId }, { {connection_end_id_t(0), { busId, connection_end_id_t(6) }} } };
			for (connection_end_id_t::rep portId = 0; portId < 6; ++portId) {
				replacer.makeConnection(pauseGuard, EvalConnection(EvalConnectionPoint(busId, connection_end_id_t(portId)), EvalConnectionPoint(gateId, connection_end_id_t(portId))));
			}
			gatesWithLinkedIO[gateId] = std::move(gateWithLinkedIO);
			return true;
		}
		auto configIter = blockTypesWithlinkedIO.find(blockType);
		if (configIter == blockTypesWithlinkedIO.end()) {
			return false;
		}
		replacer.addGate(pauseGuard, blockType, gateId);
		GateWithLinkedIO gateWithLinkedIO { gateId, {}, {} };
		for (connection_end_id_t portId : configIter->second) {
			middle_id_t junctionId = middleIdProvider.getNewId();
			replacer.addGate(pauseGuard, BlockType::JUNCTION, junctionId);
			replacer.makeConnection(pauseGuard, EvalConnection(EvalConnectionPoint(junctionId, connection_end_id_t(0)), EvalConnectionPoint(gateId, portId)));
			gateWithLinkedIO.idsCreated.push_back(junctionId);
			gateWithLinkedIO.linkedIO[portId] = EvalConnectionPoint(junctionId, connection_end_id_t(0));
		}
		gatesWithLinkedIO[gateId] = std::move(gateWithLinkedIO);
		return true;
	}
	void deleteGateWithLinkedIO(SimPauseGuard& pauseGuard, middle_id_t gateId) {
		if (!gatesWithLinkedIO.contains(gateId)) {
			return;
		}
		GateWithLinkedIO& gate = gatesWithLinkedIO.at(gateId);
		for (middle_id_t middleId : gate.idsCreated) {
			replacer.removeGate(pauseGuard, middleId);
			middleIdProvider.releaseId(middleId);
		}
		gatesWithLinkedIO.erase(gateId);
	}
	void redirectConnectionToLinked(EvalConnection& connection) {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		middle_id_t sourceGateId = connection.source.gateId;
		middle_id_t destinationGateId = connection.destination.gateId;
		if (gatesWithLinkedIO.contains(destinationGateId)) {
			GateWithLinkedIO& gate = gatesWithLinkedIO.at(destinationGateId);
			if (gate.linkedIO.contains(connection.destination.portId)) {
				connection.destination = gate.linkedIO.at(connection.destination.portId);
			}
		}
		if (gatesWithLinkedIO.contains(sourceGateId)) {
			GateWithLinkedIO& gate = gatesWithLinkedIO.at(sourceGateId);
			if (gate.linkedIO.contains(connection.source.portId)) {
				connection.source = gate.linkedIO.at(connection.source.portId);
			}
		}
	}

	std::map<BlockType, std::vector<connection_end_id_t>> blockTypesWithlinkedIO = {
		{BlockType::TRISTATE_BUFFER, {connection_end_id_t(0), connection_end_id_t(1)}},
		{BlockType::NOT, {connection_end_id_t(0)}},
		{BlockType::BUFFER, {connection_end_id_t(0)}}
	};
};

#endif /* gateSubstituter_h */
