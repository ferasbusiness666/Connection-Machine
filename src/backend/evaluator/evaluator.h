#ifndef evaluator_h
#define evaluator_h

#include "util/evalConfig.h"
#include "simulator/logicState.h"
#include "backend/address.h"
#include "backend/circuit/circuit.h"
#include "backend/circuit/circuitBlockDataManager.h"
#include "simulator/evalLogicSimulator.h"

class EvaluatorInternal;
class DataUpdateEventManager;
class CircuitManager;

class Evaluator {
public:
	typedef std::tuple<BlockType, connection_end_id_t, Position> RemoveCircuitIOData; // I hate tuples, but this is how I get the data
	typedef std::pair<BlockType, connection_end_id_t> SetCircuitIOData;

	Evaluator(
		evaluator_id_t evaluatorId,
		CircuitManager& circuitManager,
		BlockDataManager& blockDataManager,
		CircuitBlockDataManager& circuitBlockDataManager,
		circuit_id_t circuitId,
		DataUpdateEventManager& dataUpdateEventManager
	);
	Evaluator(const Evaluator&) = delete;
    Evaluator& operator=(const Evaluator&) = delete;
	~Evaluator();

	static constexpr double MIN_TICKRATE_DECREASABLE = 0.1;

	inline evaluator_id_t getEvaluatorId() const { return evaluatorId; }
	std::string getEvaluatorName() const;

	void resetStates() { evalLogicSimulator.resetStates(); }
	void setPause(bool pause) { evalConfig.setRunning(!pause); }
	bool isPause() const { return !evalConfig.isRunning(); }
	void addSprint(unsigned int nTicks) { evalConfig.addSprint(nTicks); }
	bool isSprinting() const { return evalConfig.getSprintCount() > 0; }
	void waitForSprintComplete() {}
	void tickStep(unsigned int nTicks) {
		setPause(true);
		evalConfig.addSprint(nTicks);
		waitForSprintComplete();
	}
	void tickStep() { tickStep(1); }
	bool stepBack() { return evalLogicSimulator.stepBack(); }
	void stepForward() { evalLogicSimulator.stepForward(); }
	bool skipBack() { return evalLogicSimulator.skipBack(); }
	bool skipForward() { return evalLogicSimulator.skipForward(); }
	bool isViewingReplay() const { return evalLogicSimulator.isViewingReplay(); }
	void setRealistic(bool realistic) { evalConfig.setRealistic(realistic); }
	bool isRealistic() const { return evalConfig.isRealistic(); }
	void setTickrate(double tickrate) { evalConfig.setTargetTickrate(tickrate); }
	double getTickrate() const { return evalConfig.getTargetTickrate(); }
	void increaseTickrateSeq() {}
	void decreaseTickrateSeq() {}
	void setUseTickrate(bool useTickrate) { evalConfig.setTickrateLimiter(useTickrate); }
	bool getUseTickrate() const { return evalConfig.isTickrateLimiterEnabled(); }
	double getRealTickrate() const { return evalLogicSimulator.getAverageTickrate(); }
	void makeEdit(DifferenceSharedPtr difference, circuit_id_t circuitId);
	logic_state_t getState(const Address& address) const;
	void setState(const Address& address, logic_state_t state);
	circuit_id_t getCircuitId() const { return circuitId; }
	circuit_id_t getCircuitId(const Address& address) const { if (address.size() == 0) return circuitId; return 0; }
	// const EvalAddressTree buildAddressTree() const;
	// const EvalAddressTree buildAddressTree(eval_circuit_id_t evalCircuitId) const;

	simulator_id_t getBlockSimulatorId(const Address& address) const;
	std::variant<simulator_id_t, std::vector<simulator_id_t>> getPinSimulatorId(const Address& address) const { return 0; }

	std::vector<simulator_id_t> getBlockSimulatorIds(const Address& addressOrigin, const std::vector<Position>& positions) const;
	std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> getPinSimulatorIds(const Address& addressOrigin, const std::vector<Position>& positions) const { return {0}; }

	logic_state_t getStateFromSimulatorId(simulator_id_t simulatorId) const { return evalLogicSimulator.getState(simulatorId); }
	std::vector<logic_state_t> getStatesFromSimulatorIds(const std::vector<simulator_id_t>& simulatorIds) const { return evalLogicSimulator.getStates(simulatorIds); }

	nlohmann::json dumpState() const;

	void connectListener(
		void* object,
		const Address& address,
		SimulatorMappingUpdateListenerFunction func
	) { evalLogicSimulator.connectListener(object, address, func); }
	void disconnectListener(void* object) { evalLogicSimulator.disconnectListener(object); }

private:
	circuit_id_t circuitId;
	evaluator_id_t evaluatorId;
	CircuitManager& circuitManager;
	BlockDataManager& blockDataManager;
	CircuitBlockDataManager& circuitBlockDataManager;
	DataUpdateEventManager& dataUpdateEventManager;
	DataUpdateEventManager::DataUpdateEventReceiver receiver;
	EvalConfig evalConfig;
	std::unique_ptr<EvaluatorInternal> evaluatorInternal;
	EvalLogicSimulator evalLogicSimulator;
};

#endif /* evaluator_h */
