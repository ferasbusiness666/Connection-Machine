#ifndef evalSimulator_h
#define evalSimulator_h

#include "gateSubstituter.h"

class EvalSimulator {
public:
	EvalSimulator(
		EvalConfig& evalConfig,
		IdProvider<middle_id_t>& middleIdProvider,
		std::vector<simulator_id_t>& dirtySimulatorIds,
		std::vector<middle_id_t>& dirtyMiddleIds,
		BlockDataManager& blockDataManager
	) :
		gateSubstituter(evalConfig, middleIdProvider, dirtySimulatorIds, dirtyMiddleIds, blockDataManager) {}
	inline SimPauseGuard beginEdit() {
		logInfo("EvalSimulator::beginEdit");
		return gateSubstituter.beginEdit();
	}
	inline void endEdit(SimPauseGuard& pauseGuard) {
		logInfo("EvalSimulator::endEdit");
		gateSubstituter.endEdit(pauseGuard);
	}
	inline void addGate(SimPauseGuard& pauseGuard, const BlockType blockType, const middle_id_t gateId) {
		logInfo("EvalSimulator::addGate {} {}", "", blockType, gateId);
		gateSubstituter.addGate(pauseGuard, blockType, gateId);
	}
	inline void removeGate(SimPauseGuard& pauseGuard, const middle_id_t gateId) {
		logInfo("EvalSimulator::removeGate {}", "", gateId);
		gateSubstituter.removeGate(pauseGuard, gateId);
	}
	inline logic_state_t getState(EvalConnectionPoint point) const {
		return gateSubstituter.getState(point);
	}
	inline std::vector<logic_state_t> getStates(const std::vector<EvalConnectionPoint>& points) const {
		return gateSubstituter.getStates(points);
	}
	inline std::vector<logic_state_t> getPinStates(const std::vector<EvalConnectionPoint>& points) const {
		return gateSubstituter.getPinStates(points);
	}
	inline std::vector<logic_state_t> getStatesFromSimulatorIds(const std::vector<simulator_id_t>& simulatorIds) const {
		return gateSubstituter.getStatesFromSimulatorIds(simulatorIds);
	}
	inline std::vector<simulator_id_t> getBlockSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
		return gateSubstituter.getBlockSimulatorIds(points);
	}
	inline std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> getPinSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
		return gateSubstituter.getPinSimulatorIds(points);
	}
	inline void setState(EvalConnectionPoint point, logic_state_t state) {
		gateSubstituter.setState(point, state);
	}
	inline void makeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
		logInfo("EvalSimulator::makeConnection {}", "", connection.toString());
		if (connection.toString() == "EC( (32, 0) -> (24, 0) )") {
			logInfo("BREAK");
		}
		gateSubstituter.makeConnection(pauseGuard, connection);
	}
	inline void removeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
		logInfo("EvalSimulator::removeConnection {}", "", connection.toString());
		gateSubstituter.removeConnection(pauseGuard, connection);
	}
	inline double getAverageTickrate() const {
		return gateSubstituter.getAverageTickrate();
	}
private:
	GateSubstituter gateSubstituter;
};

#endif /* evalSimulator_h */
