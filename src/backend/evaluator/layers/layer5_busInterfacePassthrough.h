#ifndef busInterfacePassthrough_h
#define busInterfacePassthrough_h

#include "backend/blockData/blockDataManager.h"
#include "layer6_simulatorOptimizer.h"

class BusInterfacePassthrough {
public:
	BusInterfacePassthrough(
		EvalConfig& evalConfig,
		IdProvider<middle_id_t>& middleIdProvider,
		std::vector<simulator_id_t>& dirtySimulatorIds,
		BlockDataManager& blockDataManager
	) : simulatorOptimizer(evalConfig, middleIdProvider, dirtySimulatorIds),
	dirtySimulatorIds(dirtySimulatorIds),
	blockDataManager(blockDataManager) {}

	void resetStates() { simulatorOptimizer.resetStates(); }

	void addGate(SimPauseGuard& pauseGuard, const BlockType blockType, const middle_id_t gateId) {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		if (gateId == 466) {
			logInfo("Adding gate {} of type {}", "BusInterfacePassthrough::addGate", gateId, blocktype_to_string(blockType));
			if (blockType == BlockType::JUNCTION) {
				logInfo("It's a junction!", "BusInterfacePassthrough::addGate");
			}
		}
		BlockData* blockData = blockDataManager.getBlockData(blockType);
		if (blockData->isBus()) {
			busInterfaces[gateId] = blockType;
			return;
		}
		simulatorOptimizer.addGate(pauseGuard, blockType, gateId);
	}

	void removeGate(SimPauseGuard& pauseGuard, const middle_id_t gateId) {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
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
		}
		if (busInterfaces.contains(gateId)) {
			busInterfaces.erase(gateId);
		} else {
			simulatorOptimizer.removeGate(pauseGuard, gateId);
		}
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

	inline logic_state_t getStateFromSimulatorId(simulator_id_t simulatorId) const {
		return simulatorOptimizer.getStateFromSimulatorId(simulatorId);
	}

	inline std::vector<logic_state_t> getStatesFromSimulatorIds(const std::vector<simulator_id_t>& simulatorIds) const {
		return simulatorOptimizer.getStatesFromSimulatorIds(simulatorIds);
	}

	inline simulator_id_t getBlockSimulatorId(EvalConnectionPoint point) const {
		if (busInterfaces.contains(point.gateId)) {
			return simulator_id_t(0);
		}
		return simulatorOptimizer.getBlockSimulatorId(point);
	}

	inline simulator_id_t getPinSimulatorId(EvalConnectionPoint point) const {
		if (busInterfaces.contains(point.gateId)) {
			return simulator_id_t(0);
		}
		return simulatorOptimizer.getPinSimulatorId(point);
	}

	inline std::vector<simulator_id_t> getBlockSimulatorIds(const std::vector<EvalConnectionPoint>& points) const {
		return simulatorOptimizer.getBlockSimulatorIds(points);
	}

	inline std::vector<simulator_id_t> getPinSimulatorIds(const std::vector<EvalConnectionPoint>& points) const {
		return simulatorOptimizer.getPinSimulatorIds(points);
	}

	inline std::vector<simulator_id_t> getBlockSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
		return simulatorOptimizer.getBlockSimulatorIds(points);
	}

	inline std::vector<simulator_id_t> getPinSimulatorIds(const std::vector<std::optional<EvalConnectionPoint>>& points) const {
		return simulatorOptimizer.getPinSimulatorIds(points);
	}

	inline void setState(simulator_id_t simulatorId, logic_state_t state) {
		simulatorOptimizer.setState(simulatorId, state);
	}

	void makeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		if (busInterfaces.contains(connection.source.gateId) ||
			busInterfaces.contains(connection.destination.gateId)) {
			omittedConnections[connection.source.gateId].push_back(connection);
			omittedConnections[connection.destination.gateId].push_back(connection);
			return;
		}
		simulatorOptimizer.makeConnection(pauseGuard, connection);
	}

	void removeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		if (busInterfaces.contains(connection.source.gateId) ||
			busInterfaces.contains(connection.destination.gateId)) {
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

	BlockType getBlockType(middle_id_t middleId) const {
		if (busInterfaces.contains(middleId)) {
			return busInterfaces.at(middleId);
		}
		return simulatorOptimizer.getBlockType(middleId);
	}

	std::pair<const std::vector<EvalConnection>&, std::vector<EvalConnection>> getInputs(middle_id_t middleId) const {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		static std::vector<EvalConnection> emptyConns;
		std::vector<EvalConnection> conns = {};
		if (omittedConnections.contains(middleId)) {
			for (const EvalConnection& conn : omittedConnections.at(middleId)) {
				if (conn.destination.gateId == middleId) {
					conns.push_back(conn);
				}
			}
		}
		if (busInterfaces.contains(middleId)) {
			return {emptyConns, conns};
		}
		return {simulatorOptimizer.getInputs(middleId), conns};
	}

	std::pair<const std::vector<EvalConnection>&, std::vector<EvalConnection>> getOutputs(middle_id_t middleId) const {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		static std::vector<EvalConnection> emptyConns;
		std::vector<EvalConnection> conns = {};
		if (omittedConnections.contains(middleId)) {
			for (const EvalConnection& conn : omittedConnections.at(middleId)) {
				if (conn.source.gateId == middleId) {
					conns.push_back(conn);
				}
			}
		}
		if (busInterfaces.contains(middleId)) {
			return {emptyConns, conns};
		}
		return {simulatorOptimizer.getOutputs(middleId), conns};
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
		if (busInterfaces.contains(middleId)) {
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
		if (busInterfaces.contains(middleId)) {
			return count;
		}
		return count + simulatorOptimizer.getNumInputs(middleId);
	}
	inline bool stepBack() {
		return simulatorOptimizer.stepBack();
	}
	inline bool stepForward() {
		return simulatorOptimizer.stepForward();
	}
	inline bool skipBack() {
		return simulatorOptimizer.skipBack();
	}
	inline bool skipForward() {
		return simulatorOptimizer.skipForward();
	}
	inline bool isViewingReplay() const {
		return simulatorOptimizer.isViewingReplay();
	}

	nlohmann::json dumpState() const /* GCOVR_EXCL_FUNCTION */ {
		nlohmann::json stateJson;
		stateJson["simulatorOptimizer"] = simulatorOptimizer.dumpState();
		stateJson["omittedConnections"] = nlohmann::json::object();
		for (const auto& [gateId, connections] : omittedConnections) {
			stateJson["omittedConnections"][std::to_string(gateId.get())] = dumpEvalConnectionVector(connections);
		}
		stateJson["busInterfaces"] = nlohmann::json::object();
		for (const auto& [gateId, blockType] : busInterfaces) {
			stateJson["busInterfaces"][std::to_string(gateId.get())] = blocktype_to_string(blockType);
		}
		return stateJson;
	}

private:
	SimulatorOptimizer simulatorOptimizer;
	std::unordered_map<middle_id_t, std::vector<EvalConnection>> omittedConnections;
	std::unordered_map<middle_id_t, BlockType> busInterfaces;
	std::vector<simulator_id_t>& dirtySimulatorIds;
	BlockDataManager& blockDataManager;

	nlohmann::json dumpEvalConnectionVector(const std::vector<EvalConnection>& connections) const /* GCOVR_EXCL_FUNCTION */ {
		nlohmann::json connArray = nlohmann::json::array();
		for (const EvalConnection& conn : connections) {
			connArray.push_back(conn.dumpState());
		}
		return connArray;
	}
};

#endif /* busInterfacePassthrough_h */