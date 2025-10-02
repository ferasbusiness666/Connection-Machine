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
        if (gateType == GateType::BUS_INTERFACE) {
            busInterfaceIds.insert(gateId);
            return;
        }
        simulatorOptimizer.addGate(pauseGuard, gateType, gateId);
    }

    void removeGate(SimPauseGuard& pauseGuard, const middle_id_t gateId) {
        if (omittedConnections.contains(gateId)) {
            std::vector<EvalConnection> omittedConns = omittedConnections[gateId];
            for (const EvalConnection& conn : omittedConns) {
                omittedConnections.at(conn.source.gateId).erase(
                    std::remove_if(
                        omittedConnections.at(conn.source.gateId).begin(),
                        omittedConnections.at(conn.source.gateId).end(),
                        [&](const EvalConnection& c) { return c == conn; }
                    ),
                    omittedConnections.at(conn.source.gateId).end()
                );
                omittedConnections.at(conn.destination.gateId).erase(
                    std::remove_if(
                        omittedConnections.at(conn.destination.gateId).begin(),
                        omittedConnections.at(conn.destination.gateId).end(),
                        [&](const EvalConnection& c) { return c == conn; }
                    ),
                    omittedConnections.at(conn.destination.gateId).end()
                );
            }
            omittedConnections.erase(gateId);
            if (busInterfaceIds.contains(gateId)) {
                busInterfaceIds.erase(gateId);
            }
            return;
        }
        if (busInterfaceIds.contains(gateId)) {
            busInterfaceIds.erase(gateId);
            return;
        }
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
        return simulatorOptimizer.getPinSimulatorIds(points);
    }

    inline void setState(EvalConnectionPoint point, logic_state_t state) {
        simulatorOptimizer.setState(point, state);
    }

    void makeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
        if (busInterfaceIds.contains(connection.source.gateId) ||
            busInterfaceIds.contains(connection.destination.gateId)) {
            omittedConnections[connection.source.gateId].push_back(connection);
            omittedConnections[connection.destination.gateId].push_back(connection);
            return;
        }
        simulatorOptimizer.makeConnection(pauseGuard, connection);
    }

    void removeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
        if (busInterfaceIds.contains(connection.source.gateId) ||
            busInterfaceIds.contains(connection.destination.gateId)) {
            omittedConnections[connection.source.gateId].erase(
                std::remove_if(
                    omittedConnections[connection.source.gateId].begin(),
                    omittedConnections[connection.source.gateId].end(),
                    [&](const EvalConnection& c) { return c == connection; }
                ),
                omittedConnections[connection.source.gateId].end()
            );
            omittedConnections[connection.destination.gateId].erase(
                std::remove_if(
                    omittedConnections[connection.destination.gateId].begin(),
                    omittedConnections[connection.destination.gateId].end(),
                    [&](const EvalConnection& c) { return c == connection; }
                ),
                omittedConnections[connection.destination.gateId].end()
            );
            return;
        }
        simulatorOptimizer.removeConnection(pauseGuard, connection);
    }

    inline double getAverageTickrate() const {
        return simulatorOptimizer.getAverageTickrate();
    }

    GateType getGateType(middle_id_t middleId) const {
        if (busInterfaceIds.contains(middleId)) {
            return GateType::BUS_INTERFACE;
        }
        return simulatorOptimizer.getGateType(middleId);
    }

    std::vector<EvalConnection> getInputs(middle_id_t middleId) const {
        std::vector<EvalConnection> conns = {};
        if (omittedConnections.contains(middleId)) {
            for (const EvalConnection& conn : omittedConnections.at(middleId)) {
                if (conn.destination.gateId == middleId) {
                    conns.push_back(conn);
                }
            }
        }
        if (busInterfaceIds.contains(middleId)) {
            return conns;
        }
        for (const EvalConnection& conn : simulatorOptimizer.getInputs(middleId)) {
            conns.push_back(conn);
        }
        return conns;
    }

    std::vector<EvalConnection> getOutputs(middle_id_t middleId) const {
        std::vector<EvalConnection> conns = {};
        if (omittedConnections.contains(middleId)) {
            for (const EvalConnection& conn : omittedConnections.at(middleId)) {
                if (conn.source.gateId == middleId) {
                    conns.push_back(conn);
                }
            }
        }
        if (busInterfaceIds.contains(middleId)) {
            return conns;
        }
        for (const EvalConnection& conn : simulatorOptimizer.getOutputs(middleId)) {
            conns.push_back(conn);
        }
        return conns;
    }
    int getNumOutputs(middle_id_t middleId) const {
        int count = 0;
        if (omittedConnections.contains(middleId)) {
            for (const EvalConnection& conn : omittedConnections.at(middleId)) {
                if (conn.source.gateId == middleId) {
                    count++;
                }
            }
        }
        if (busInterfaceIds.contains(middleId)) {
            return count;
        }
        return count + simulatorOptimizer.getNumOutputs(middleId);
    }
    int getNumInputs(middle_id_t middleId) const {
        int count = 0;
        if (omittedConnections.contains(middleId)) {
            for (const EvalConnection& conn : omittedConnections.at(middleId)) {
                if (conn.destination.gateId == middleId) {
                    count++;
                }
            }
        }
        if (busInterfaceIds.contains(middleId)) {
            return count;
        }
        return count + simulatorOptimizer.getNumInputs(middleId);
    }
private:
    SimulatorOptimizer simulatorOptimizer;
    std::unordered_map<middle_id_t, std::vector<EvalConnection>> omittedConnections;
    std::unordered_set<middle_id_t> busInterfaceIds;
};

#endif /* busInterfacePassthrough_h */