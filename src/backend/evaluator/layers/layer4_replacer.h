#ifndef replacer_h
#define replacer_h

#include "layer5_busInterfacePassthrough.h"
#include "layer4_replacement.h"

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
		std::vector<simulator_id_t>& dirtySimulatorIds,
		std::vector<middle_id_t>& dirtyMiddleIds,
		BlockDataManager& blockDataManager) :
		busInterfacePassthrough(evalConfig, middleIdProvider, dirtySimulatorIds, blockDataManager),
		evalConfig(evalConfig),
		dirtyMiddleIds(dirtyMiddleIds),
		middleIdProvider(middleIdProvider),
		blockDataManager(blockDataManager) {}

	void addGate(SimPauseGuard& pauseGuard, const BlockType blockType, const middle_id_t gateId) {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		busInterfacePassthrough.addGate(pauseGuard, blockType, gateId);
		if (isJunctionType(blockType)) {
			existingJunctionIds.insert(gateId);
		}
	}

	void removeGate(SimPauseGuard& pauseGuard, const middle_id_t gateId) {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		pingId(pauseGuard, gateId, 0);
		busInterfacePassthrough.removeGate(pauseGuard, gateId);
		existingJunctionIds.erase(gateId);
	}

	inline SimPauseGuard beginEdit() {
		return busInterfacePassthrough.beginEdit();
	}

	void endEdit(SimPauseGuard& pauseGuard) {
		cleanReplacements();
		mergeBuses(pauseGuard, 0, 2);
		mergeJunctions(pauseGuard, 1);

		busInterfacePassthrough.endEdit(pauseGuard);
	}

	inline std::optional<simulator_id_t> getSimIdFromMiddleId(middle_id_t middleId) const {
		return busInterfacePassthrough.getSimIdFromMiddleId(middleId);
	}

	inline logic_state_t getStateFromSimulatorId(simulator_id_t simulatorId) const {
		return busInterfacePassthrough.getStateFromSimulatorId(simulatorId);
	}

	inline std::vector<logic_state_t> getStatesFromSimulatorIds(const std::vector<simulator_id_t>& simulatorIds) const {
		return busInterfacePassthrough.getStatesFromSimulatorIds(simulatorIds);
	}

	inline simulator_id_t getBlockSimulatorId(EvalConnectionPoint point) const {
		BlockType originalBlockType = busInterfacePassthrough.getBlockType(point.gateId);
		if (originalBlockType != BlockType::NONE) {
			const BlockData* blockData = blockDataManager.getBlockData(originalBlockType);
			if (!blockData->hasBlockState()) {
				return simulator_id_t(0);
			}
		}
		EvalConnectionPoint replacedPoint = getReplacementConnectionPoint(point);
		BlockType blockType = busInterfacePassthrough.getBlockType(replacedPoint.gateId);
		if (blockType == BlockType::NONE) {
			return simulator_id_t(3);
		}
		return busInterfacePassthrough.getBlockSimulatorId(replacedPoint);
	}

	std::variant<simulator_id_t, std::vector<simulator_id_t>> getPinSimulatorId(EvalConnectionPoint point) const;

	inline void setState(simulator_id_t simulatorId, logic_state_t state) {
		busInterfacePassthrough.setState(simulatorId, state);
	}

	void makeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		pingId(pauseGuard, connection.source.gateId, 0);
		pingId(pauseGuard, connection.destination.gateId, 0);
		busInterfacePassthrough.makeConnection(pauseGuard, connection);
	}

	void removeConnection(SimPauseGuard& pauseGuard, EvalConnection connection) {
#ifdef TRACY_PROFILER
		ZoneScoped;
#endif
		pingId(pauseGuard, connection.source.gateId, 0);
		pingId(pauseGuard, connection.destination.gateId, 0);
		busInterfacePassthrough.removeConnection(pauseGuard, connection);
		busInterfacePassthrough.removeConnection(pauseGuard, connection.reverse());
	}

	inline double getAverageTickrate() const {
		return busInterfacePassthrough.getAverageTickrate();
	}
	inline bool stepBack() {
		return busInterfacePassthrough.stepBack();
	}
	inline bool stepForward() {
		return busInterfacePassthrough.stepForward();
	}
	inline bool skipBack() {
		return busInterfacePassthrough.skipBack();
	}
	inline bool skipForward() {
		return busInterfacePassthrough.skipForward();
	}
	inline bool isViewingReplay() const {
		return busInterfacePassthrough.isViewingReplay();
	}

	nlohmann::json dumpState() const;

private:
	BusInterfacePassthrough busInterfacePassthrough;
	EvalConfig& evalConfig;
	IdProvider<middle_id_t>& middleIdProvider;
	IdVector<replacement_id_t, std::optional<Replacement>> replacements;
	IdProvider<replacement_id_t> replacementIdProvider { 0 };
	std::unordered_map<middle_id_t, std::unordered_map<connection_end_id_t, EvalConnectionPoint>> replacedConnectionPoints;
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
		logic_state_t defaultState { logic_state_t::FLOATING };
	};

	struct BusFloodFillResult {
		std::vector<middle_id_t> busIds;
		std::vector<middle_id_t> junctionIds;
		std::vector<EvalConnection> connectionsBetweenBusesAndJunctions;
		std::vector<EvalConnection> connectionsIntoBuses;
		std::vector<EvalConnection> connectionsOutOfBuses;
	};

	Replacement& makeReplacement(int layer);
	void cleanReplacements();
	void pingId(SimPauseGuard& pauseGuard, middle_id_t id, int minLayer);
	EvalConnectionPoint getReplacementConnectionPoint(EvalConnectionPoint point) const;
	std::vector<EvalConnectionPoint> getReplacementConnectionPoints(const std::vector<EvalConnectionPoint>& points) const;
	std::vector<std::optional<EvalConnectionPoint>> getReplacementConnectionPoints(const std::vector<std::optional<EvalConnectionPoint>>& points) const;
	struct BusInternalJunctionArray {
		std::vector<middle_id_t> junctionIds {};
		int numDefined { 0 };
		nlohmann::json dumpState() const;
	};
	std::unordered_map<middle_id_t, BusInternalJunctionArray> busInternalJunctions;
	std::optional<middle_id_t> getJunctionInsideBus(middle_id_t busId, unsigned int laneId) {
		if (!busInternalJunctions.contains(busId)) {
			return std::nullopt;
		}
		const BusInternalJunctionArray& busInternalJunctionArray = busInternalJunctions.at(busId);
		if (busInternalJunctionArray.junctionIds.size() <= laneId) {
			return std::nullopt;
		}
		middle_id_t junctionId = busInternalJunctionArray.junctionIds[laneId];
		if (junctionId == middle_id_t(0)) {
			return std::nullopt;
		}
		return junctionId;
	};
	void defineJunctionInsideBus(middle_id_t busId, unsigned int laneId, middle_id_t junctionId) {
		dirtyMiddleIds.push_back(busId);
		if (!busInternalJunctions.contains(busId)) {
			busInternalJunctions[busId] = { std::vector<middle_id_t>{}, 0 };
		}
		BusInternalJunctionArray& busInternalJunctionArray = busInternalJunctions.at(busId);
		if (busInternalJunctionArray.junctionIds.size() <= laneId) {
			busInternalJunctionArray.junctionIds.resize(laneId + 1, middle_id_t(0));
		}
		busInternalJunctionArray.junctionIds[laneId] = junctionId;
		busInternalJunctionArray.numDefined += 1;
	};
	void defineJunctionInsideBus(middle_id_t busId, unsigned int laneId, middle_id_t junctionId, Replacement& replacement) {
		defineJunctionInsideBus(busId, laneId, junctionId);
		replacement.addRevertAction([this, busId, laneId]() {
			undefineJunctionInsideBus(busId, laneId);
		});
	};
	void undefineJunctionInsideBus(middle_id_t busId, unsigned int laneId) {
		dirtyMiddleIds.push_back(busId);
		BusInternalJunctionArray& busInternalJunctionArray = busInternalJunctions.at(busId);
		busInternalJunctionArray.numDefined -= 1;
		busInternalJunctionArray.junctionIds[laneId] = middle_id_t(0);
		if (busInternalJunctionArray.numDefined == 0) {
			busInternalJunctions.erase(busId);
			return;
		}
	};

	struct BlockLane {
		middle_id_t blockId;
		unsigned int laneId;
		bool operator==(const BlockLane& other) const noexcept {
			return blockId == other.blockId && laneId == other.laneId;
		}
		bool operator!=(const BlockLane& other) const noexcept {
			return !(*this == other);
		}
		bool operator<(const BlockLane& other) const noexcept {
			return std::tie(blockId, laneId) < std::tie(other.blockId, other.laneId);
		}
		struct Hash {
			size_t operator()(const BlockLane& value) const noexcept {
				size_t seed = static_cast<size_t>(value.blockId.get());
				seed ^= static_cast<size_t>(value.laneId) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
				return seed;
			}
		};
	};

	void mergeBuses(SimPauseGuard& pauseGuard, int layer, int junctionOverpowerLayer);
	void mergeBusLane(SimPauseGuard& pauseGuard, int layer, int junctionOverpowerLayer, middle_id_t id, unsigned int laneId);
	void mergeJunctions(SimPauseGuard& pauseGuard, int layer);
	JunctionFloodFillResult junctionFloodFill(middle_id_t junctionId);
	BlockDataManager& blockDataManager;
	std::vector<middle_id_t>& dirtyMiddleIds;
	static bool isJunctionType(BlockType blockType) {
		return blockType == BlockType::JUNCTION || blockType == BlockType::JUNCTION_L || blockType == BlockType::JUNCTION_H || blockType == BlockType::JUNCTION_X;
	}
	std::unordered_map<middle_id_t, std::unordered_set<replacement_id_t>> dependentReplacements;

	nlohmann::json dumpConnectionPointMap(const std::unordered_map<connection_end_id_t, EvalConnectionPoint>& pointMap) const;
	nlohmann::json dumpReplacementIdSet(const std::unordered_set<replacement_id_t>& idSet) const;
	std::unordered_set<middle_id_t> existingJunctionIds;
};

#endif /* replacer_h */
