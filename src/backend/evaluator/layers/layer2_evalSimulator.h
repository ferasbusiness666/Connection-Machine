#ifndef evalSimulator_h
#define evalSimulator_h

#include "layer3_gateSubstituter.h"

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
	inline void resetStates() {
		gateSubstituter.resetStates();
	}
	inline SimPauseGuard beginEdit() {
		return gateSubstituter.beginEdit();
	}
	inline void endEdit(SimPauseGuard& pauseGuard) {
#ifdef TRACY_PROFILER
	ZoneScoped;
#endif
		gateSubstituter.endEdit(pauseGuard);
	}
	inline void addGate(SimPauseGuard& pauseGuard, const BlockType blockType, const middle_id_t gateId) {
		gateSubstituter.addGate(pauseGuard, blockType, gateId);
	}
	inline void removeGate(SimPauseGuard& pauseGuard, const middle_id_t gateId) {
		gateSubstituter.removeGate(pauseGuard, gateId);
	}
	inline logic_state_t getStateFromSimulatorId(simulator_id_t simulatorId) const {
		return gateSubstituter.getStateFromSimulatorId(simulatorId);
	}
	inline std::vector<logic_state_t> getStatesFromSimulatorIds(const std::vector<simulator_id_t>& simulatorIds) const {
		return gateSubstituter.getStatesFromSimulatorIds(simulatorIds);
	}
	inline simulator_id_t getBlockSimulatorId(EvalConnectionPoint point) const {
		return gateSubstituter.getBlockSimulatorId(point);
	}
	inline std::variant<simulator_id_t, std::vector<simulator_id_t>> getPinSimulatorId(EvalConnectionPoint point) const {
		return gateSubstituter.getPinSimulatorId(point);
	}
	inline void setState(simulator_id_t simulatorId, logic_state_t state) {
		gateSubstituter.setState(simulatorId, state);
	}
	inline void makeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
		gateSubstituter.makeConnection(pauseGuard, connection);
	}
	inline void removeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
		gateSubstituter.removeConnection(pauseGuard, connection);
	}
	inline double getAverageTickrate() const {
		return gateSubstituter.getAverageTickrate();
	}
	inline bool stepBack() {
		return gateSubstituter.stepBack();
	}
	inline bool stepForward() {
		return gateSubstituter.stepForward();
	}
	inline bool skipBack() {
		return gateSubstituter.skipBack();
	}
	inline bool skipForward() {
		return gateSubstituter.skipForward();
	}
	inline bool isViewingReplay() const {
		return gateSubstituter.isViewingReplay();
	}
	nlohmann::json dumpState() const /* GCOVR_EXCL_FUNCTION */ {
		nlohmann::json stateJson;
		stateJson["gateSubstituter"] = gateSubstituter.dumpState();
		return stateJson;
	}
private:
	GateSubstituter gateSubstituter;
};

#endif /* evalSimulator_h */
