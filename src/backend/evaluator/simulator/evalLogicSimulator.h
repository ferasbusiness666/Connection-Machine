#ifndef evalLogicSimulator_h
#define evalLogicSimulator_h

#include "logicSimulator.h"

class EvaluatorInternal;
class BlockDataManager;
class Address;

class EvalLogicSimulator {
public:
	EvalLogicSimulator(EvalConfig& evalConfig, const BlockDataManager& blockDataManager, const EvaluatorInternal& evaluatorInternal);

	void clearState() { logicSimulator.clearState(); }
	double getAverageTickrate() const { return logicSimulator.getAverageTickrate(); }

	void resetStates() { logicSimulator.resetStates(); }

	void setState(simulator_id_t id, logic_state_t state) { logicSimulator.setState(id, state); }
	logic_state_t getState(simulator_id_t id) const { return logicSimulator.getState(id); }
	std::vector<logic_state_t> getStates(const std::vector<simulator_id_t>& ids) const { return logicSimulator.getStates(ids); }

	std::optional<simulator_id_t> getOutputPortId(eval_gate_id gateId, connection_end_id_t portId) const;

	void makeEdit(); // knows to check evaluatorInternal

	// simulator_id_t addGate(const BlockType blockType);
	// void removeGate(simulator_id_t gateId);
	// void makeConnection(simulator_id_t sourceId, connection_end_id_t sourcePort, simulator_id_t destinationId, connection_end_id_t destinationPort);
	// void removeConnection(simulator_id_t sourceId, connection_end_id_t sourcePort, simulator_id_t destinationId, connection_end_id_t destinationPort);
	// void endEdit();

	bool stepBack() { return logicSimulator.stepBack(); }
	bool stepForward() { return logicSimulator.stepForward(); }
	bool skipBack() { return logicSimulator.skipBack(); }
	bool skipForward() { return logicSimulator.skipForward(); }
	inline bool isViewingReplay() const { return logicSimulator.isViewingReplay(); }

	void connectListener(void* object, const Address& address, SimulatorMappingUpdateListenerFunction func);
	void disconnectListener(void* object) {
		auto iter = simulatorMappingUpdateListeners.find(object);
		if (iter != simulatorMappingUpdateListeners.end()) {
			simulatorMappingUpdateListeners.erase(iter);
		}
	}

	const std::unordered_map<eval_gate_id, simulator_id_t>& getGateIdMapping() const { return gateIdMapping; }

	nlohmann::json dumpState() const { return logicSimulator.dumpState(); }

private:
	std::vector<simulator_id_t> dirtySimulatorIds;
	LogicSimulator logicSimulator;
	const BlockDataManager& blockDataManager;
	const EvaluatorInternal& evaluatorInternal;

	std::unordered_map<eval_gate_id, simulator_id_t> gateIdMapping;
	std::map<void*, SimulatorMappingUpdateListener> simulatorMappingUpdateListeners;
};

#endif /* evalLogicSimulator_h */
