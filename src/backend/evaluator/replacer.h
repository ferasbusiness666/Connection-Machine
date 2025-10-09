#ifndef replacer_h
#define replacer_h

#include "busInterfacePassthrough.h"
#include "evalConfig.h"
#include "evalConnection.h"
#include "evalTypedef.h"
#include "idProvider.h"
#include "logicSimulator.h"
#include "replacement.h"

struct SimulatorStateAndPinSimId {
	std::variant<simulator_id_t, std::vector<simulator_id_t>> portSimIds;
	std::variant<simulator_id_t, std::vector<simulator_id_t>> pinSimIds;
};

class Replacer {
	friend class Replacement;
public:
	Replacer(
		EvalConfig& evalConfig,
		IdProvider<middle_id_t>& middleIdProvider,
		std::vector<simulator_id_t>& dirtySimulatorIds) :
		busInterfacePassthrough(evalConfig, middleIdProvider, dirtySimulatorIds),
		evalConfig(evalConfig),
		middleIdProvider(middleIdProvider) {}

	void addGate(SimPauseGuard& pauseGuard, const BlockType blockType, const middle_id_t gateId) {
		busInterfacePassthrough.addGate(pauseGuard, blockType, gateId);
	}

	void removeGate(SimPauseGuard& pauseGuard, const middle_id_t gateId) {
		pingOutputs(pauseGuard, gateId);
		pingInputs(pauseGuard, gateId);
		busInterfacePassthrough.removeGate(pauseGuard, gateId);
	}

	inline SimPauseGuard beginEdit() {
		return busInterfacePassthrough.beginEdit();
	}

	void endEdit(SimPauseGuard& pauseGuard) {
		cleanReplacements();
		mergeBuses(pauseGuard, 0);
		mergeJunctions(pauseGuard, 1);

		busInterfacePassthrough.endEdit(pauseGuard);
	}

	inline std::optional<simulator_id_t> getSimIdFromMiddleId(middle_id_t middleId) const {
		return busInterfacePassthrough.getSimIdFromMiddleId(middleId);
	}

	inline logic_state_t getState(EvalConnectionPoint point) const {
		return busInterfacePassthrough.getState(getReplacementConnectionPoint(point));
	}

	inline std::vector<logic_state_t> getStates(const std::vector<EvalConnectionPoint>& points) const {
		return busInterfacePassthrough.getStates(getReplacementConnectionPoints(points));
	}

	inline std::vector<logic_state_t> getPinStates(const std::vector<EvalConnectionPoint>& points) const {
		return busInterfacePassthrough.getPinStates(getReplacementConnectionPoints(points));
	}

	inline std::vector<logic_state_t> getStatesFromSimulatorIds(const std::vector<simulator_id_t>& simulatorIds) const {
		return busInterfacePassthrough.getStatesFromSimulatorIds(simulatorIds);
	}

	inline std::vector<SimulatorStateAndPinSimId> getSimulatorIds(const std::vector<EvalConnectionPoint>& points) const {
		// return busInterfacePassthrough.getSimulatorIds(getReplacementConnectionPoints(points));
		std::vector<EvalConnectionPoint> replacedPoints = getReplacementConnectionPoints(points);
		std::vector<SimulatorStateAndPinSimId> result;
		result.reserve(replacedPoints.size());
		for (const EvalConnectionPoint& point : replacedPoints) {
			if (connectionPointBusOverride.contains(point)) {
				std::vector<EvalConnectionPoint> override = getReplacementConnectionPoints(connectionPointBusOverride.at(point));
				SimulatorStateAndPinSimId simIds = {
					busInterfacePassthrough.getBlockSimulatorIds(override),
					busInterfacePassthrough.getPinSimulatorIds(override)
				};
				result.push_back(simIds);
			} else {
				SimulatorStateAndPinSimId simIds = {
					static_cast<simulator_id_t>(0),
					static_cast<simulator_id_t>(0)
				};
				simIds.portSimIds = busInterfacePassthrough.getBlockSimulatorId(point);
				simIds.pinSimIds = busInterfacePassthrough.getPinSimulatorId(point);
				result.push_back(simIds);
			}
		}
		return result;
	}

	inline std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> getBlockSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
		std::vector<std::optional<EvalConnectionPoint>> replacedPoints = getReplacementConnectionPoints(points);
		std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> result;
		result.reserve(replacedPoints.size());
		for (const auto& point : replacedPoints) {
			if (!point.has_value()) {
				result.emplace_back(static_cast<simulator_id_t>(0));
				continue;
			}
			if (connectionPointBusOverride.contains(point.value())) {
				std::vector<EvalConnectionPoint> override = getReplacementConnectionPoints(connectionPointBusOverride.at(point.value()));
				std::vector<simulator_id_t> simIds = busInterfacePassthrough.getBlockSimulatorIds(override);
				result.emplace_back(simIds);
			} else {
				simulator_id_t simId = busInterfacePassthrough.getBlockSimulatorId(*point);
				result.emplace_back(simId);
			}
		}
		return result;
	}

	inline std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> getPinSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
		// return busInterfacePassthrough.getPinSimulatorIds(getReplacementConnectionPoints(points));
		std::vector<std::optional<EvalConnectionPoint>> replacedPoints = getReplacementConnectionPoints(points);
		std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> result;
		result.reserve(replacedPoints.size());
		for (const auto& point : replacedPoints) {
			if (!point.has_value()) {
				result.emplace_back(static_cast<simulator_id_t>(0));
				continue;
			}
			if (connectionPointBusOverride.contains(point.value())) {
				std::vector<EvalConnectionPoint> override = getReplacementConnectionPoints(connectionPointBusOverride.at(point.value()));
				std::vector<simulator_id_t> simIds = busInterfacePassthrough.getPinSimulatorIds(override);
				result.emplace_back(simIds);
			} else {
				simulator_id_t simId = busInterfacePassthrough.getPinSimulatorId(*point);
				result.emplace_back(simId);
			}
		}
		return result;
	}

	inline void setState(EvalConnectionPoint id, logic_state_t state) {
		busInterfacePassthrough.setState(getReplacementConnectionPoint(id), state);
	}

	void makeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
		pingOutputs(pauseGuard, connection.source.gateId);
		pingInputs(pauseGuard, connection.destination.gateId);
		busInterfacePassthrough.makeConnection(pauseGuard, connection);
	}

	void removeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
		pingOutputs(pauseGuard, connection.source.gateId);
		pingInputs(pauseGuard, connection.destination.gateId);
		busInterfacePassthrough.removeConnection(pauseGuard, connection);
	}

	inline double getAverageTickrate() const {
		return busInterfacePassthrough.getAverageTickrate();
	}

private:
	BusInterfacePassthrough busInterfacePassthrough;
	EvalConfig& evalConfig;
	IdProvider<middle_id_t>& middleIdProvider;
	std::vector<Replacement> replacements;
	std::unordered_map<middle_id_t, std::unordered_map<connection_port_id_t, EvalConnectionPoint>> replacedConnectionPoints;
	std::unordered_map<middle_id_t, middle_id_t> replacedIds;
	std::unordered_map<middle_id_t, int> replacementIdLayers;

	struct JunctionFloodFillResult {
		std::vector<EvalConnectionPoint> outputsGoingIntoJunctions;
		// connections that are going directly into the junctions
		// A -> JUNCTION
		std::vector<EvalConnection> inputsPullingFromJunctions;
		// connections that are pulling from the junctions
		// JUNCTION -> C
		std::vector<middle_id_t> junctionIds;
		std::vector<EvalConnection> connectionsToReroute;
		// connections that are connected to the junctions through the outputs of other gates
		// A -> JUNCTION, A -> B, A -> B is a connection to reroute because B should actually pull from the junction
	};

	struct BusFloodFillResult {
		std::vector<middle_id_t> busIds;
		std::vector<middle_id_t> junctionIds;
		std::vector<EvalConnection> connectionsBetweenBusesAndJunctions;
		std::vector<EvalConnection> connectionsIntoBuses;
		std::vector<EvalConnection> connectionsOutOfBuses;
	};

	std::unordered_map<middle_id_t, std::vector<EvalConnectionPoint>> connectionPointBusOverrideLookup;
	std::unordered_map<EvalConnectionPoint, std::vector<EvalConnectionPoint>> connectionPointBusOverride;
	void addConnectionPointBusOverride(EvalConnectionPoint original, std::vector<EvalConnectionPoint> replacement) {
		connectionPointBusOverride[original] = replacement;
		if (!connectionPointBusOverrideLookup.contains(original.gateId)) {
			connectionPointBusOverrideLookup[original.gateId] = {};
		}
		connectionPointBusOverrideLookup[original.gateId].push_back(original);
	}
	void removeConnectionPointBusOverride(middle_id_t gateId) {
		if (!connectionPointBusOverrideLookup.contains(gateId)) {
			return;
		}
		for (const EvalConnectionPoint& point : connectionPointBusOverrideLookup[gateId]) {
			connectionPointBusOverride.erase(point);
		}
		connectionPointBusOverrideLookup.erase(gateId);
	}
	void removeConnectionPointBusOverride(EvalConnectionPoint point) {
		if (!connectionPointBusOverride.contains(point)) {
			return;
		}
		connectionPointBusOverride.erase(point);
		if (!connectionPointBusOverrideLookup.contains(point.gateId)) {
			return;
		}
		auto& vec = connectionPointBusOverrideLookup[point.gateId];
		vec.erase(std::remove(vec.begin(), vec.end(), point), vec.end());
		if (vec.size() == 0) {
			connectionPointBusOverrideLookup.erase(point.gateId);
		}
	}

	Replacement& makeReplacement(int layer);
	void cleanReplacements();
	void pingOutputs(SimPauseGuard& pauseGuard, middle_id_t id);
	void pingInputs(SimPauseGuard& pauseGuard, middle_id_t id);
	EvalConnectionPoint getReplacementConnectionPoint(EvalConnectionPoint point) const;
	std::vector<EvalConnectionPoint> getReplacementConnectionPoints(const std::vector<EvalConnectionPoint>& points) const;
	std::vector<std::optional<EvalConnectionPoint>> getReplacementConnectionPoints(const std::vector<std::optional<EvalConnectionPoint>>& points) const;
	void mergeBuses(SimPauseGuard& pauseGuard, int layer);
	void mergeJunctions(SimPauseGuard& pauseGuard, int layer);
	JunctionFloodFillResult junctionFloodFill(middle_id_t junctionId);
};

#endif /* replacer_h */
