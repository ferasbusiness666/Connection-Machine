#ifndef busInterfacePassthrough_h
#define busInterfacePassthrough_h

#include "gateType.h"
#include "simulatorOptimizer.h"

class BusInterfacePassthrough {
public:
    BusInterfacePassthrough(
        EvalConfig& evalConfig,
        IdProvider<middle_id_t>& middleIdProvider,
        std::vector<simulator_id_t>& dirtySimulatorIds
    ) : simulatorOptimizer(evalConfig, middleIdProvider, dirtySimulatorIds) {}

    void addGate(SimPauseGuard& pauseGuard, const GateType gateType, const middle_id_t gateId) {
        simulatorOptimizer.addGate(pauseGuard, gateType, gateId);
    }

    void removeGate(SimPauseGuard& pauseGuard, const middle_id_t gateId) {
        simulatorOptimizer.removeGate(pauseGuard, gateId);
    }

    inline SimPauseGuard beginEdit() {
        return simulatorOptimizer.beginEdit();
    }

    void endEdit(SimPauseGuard& pauseGuard) {
        simulatorOptimizer.endEdit(pauseGuard);
    }

    inline std::optional<simulator_id_t> getSimIdFromMiddleId(middle_id_t middleId) const {
        return simulatorOptimizer.getSimIdFromMiddleId(middleId);
    }

    inline std::optional<simulator_id_t> getSimIdFromConnectionPoint(const EvalConnectionPoint& point) const {
        return simulatorOptimizer.getSimIdFromConnectionPoint(point);
    }

    inline logic_state_t getState(EvalConnectionPoint point) const {
        return simulatorOptimizer.getState(point);
    }

    inline std::vector<logic_state_t> getStates(const std::vector<EvalConnectionPoint>& points) const {
        return simulatorOptimizer.getStates(points);
    }

    inline std::vector<logic_state_t> getPinStates(const std::vector<EvalConnectionPoint>& points) const {
        return simulatorOptimizer.getPinStates(points);
    }

    inline std::vector<logic_state_t> getStatesFromSimulatorIds(const std::vector<simulator_id_t>& simulatorIds) const {
        return simulatorOptimizer.getStatesFromSimulatorIds(simulatorIds);
    }

    inline std::vector<SimulatorStateAndPinSimId> getSimulatorIds(const std::vector<EvalConnectionPoint>& points) const {
        return simulatorOptimizer.getSimulatorIds(points);
    }

    inline std::vector<simulator_id_t> getBlockSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
        return simulatorOptimizer.getBlockSimulatorIds(points);
    }

    inline std::vector<simulator_id_t> getPinSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
        return simulatorOptimizer.getBlockSimulatorIds(points);
    }

    inline void setState(EvalConnectionPoint point, logic_state_t state) {
        simulatorOptimizer.setState(point, state);
    }

    void makeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
        simulatorOptimizer.makeConnection(pauseGuard, connection);
    }

    void removeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
        simulatorOptimizer.removeConnection(pauseGuard, connection);
    }

    inline double getAverageTickrate() const {
        return simulatorOptimizer.getAverageTickrate();
    }

    GateType getGateType(middle_id_t middleId) const {
        return simulatorOptimizer.getGateType(middleId);
    }

    std::vector<EvalConnection> getInputs(middle_id_t middleId) const {
        return simulatorOptimizer.getInputs(middleId);
    }

    std::vector<EvalConnection> getOutputs(middle_id_t middleId) const {
        return simulatorOptimizer.getOutputs(middleId);
    }
    int getNumOutputs(middle_id_t middleId) const {
        return simulatorOptimizer.getNumOutputs(middleId);
    }
    int getNumInputs(middle_id_t middleId) const {
        return simulatorOptimizer.getNumInputs(middleId);
    }
private:
    SimulatorOptimizer simulatorOptimizer;
};

#endif /* busInterfacePassthrough_h */