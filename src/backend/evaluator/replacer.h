#ifndef replacer_h
#define replacer_h

#include "busInterfacePassthrough.h"
#include "evalConfig.h"
#include "evalConnection.h"
#include "evalTypedef.h"
#include "idProvider.h"
#include "gateType.h"
#include "logicSimulator.h"
#include "replacement.h"

class Replacement;

class Replacer {
public:
	friend class Replacement;
	Replacer(
		EvalConfig& evalConfig,
		IdProvider<middle_id_t>& middleIdProvider,
		std::vector<simulator_id_t>& dirtySimulatorIds) :
		busInterfacePassthrough(evalConfig, middleIdProvider, dirtySimulatorIds),
		evalConfig(evalConfig),
		middleIdProvider(middleIdProvider) {}

	void addGate(SimPauseGuard& pauseGuard, const GateType gateType, const middle_id_t gateId) {
		busInterfacePassthrough.addGate(pauseGuard, gateType, gateId);
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
		mergeJunctions(pauseGuard);

		busInterfacePassthrough.endEdit(pauseGuard);
	}

	inline std::optional<simulator_id_t> getSimIdFromMiddleId(middle_id_t middleId) const {
		return busInterfacePassthrough.getSimIdFromMiddleId(middleId);
	}

	inline std::optional<simulator_id_t> getSimIdFromConnectionPoint(const EvalConnectionPoint& point) const {
		return busInterfacePassthrough.getSimIdFromConnectionPoint(point);
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
		return busInterfacePassthrough.getSimulatorIds(getReplacementConnectionPoints(points));
	}

	inline std::vector<simulator_id_t> getBlockSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
		return busInterfacePassthrough.getBlockSimulatorIds(getReplacementConnectionPoints(points));
	}

	inline std::vector<simulator_id_t> getPinSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
		return busInterfacePassthrough.getPinSimulatorIds(getReplacementConnectionPoints(points));
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
	std::unordered_set<middle_id_t> replacementIds;

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

	Replacement& makeReplacement();
	void cleanReplacements();
	void pingOutputs(SimPauseGuard& pauseGuard, middle_id_t id);
	void pingInputs(SimPauseGuard& pauseGuard, middle_id_t id);
	EvalConnectionPoint getReplacementConnectionPoint(EvalConnectionPoint point) const;
	std::vector<EvalConnectionPoint> getReplacementConnectionPoints(const std::vector<EvalConnectionPoint>& points) const;
	std::vector<std::optional<EvalConnectionPoint>> getReplacementConnectionPoints(const std::vector<std::optional<EvalConnectionPoint>>& points) const;
	void mergeJunctions(SimPauseGuard& pauseGuard);
	JunctionFloodFillResult junctionFloodFill(middle_id_t junctionId);
};

#endif /* replacer_h */
