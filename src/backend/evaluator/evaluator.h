#ifndef evaluator_h
#define evaluator_h

#include "util/evalConfig.h"
#include "simulator/logicState.h"
#include "backend/address.h"
#include "backend/circuit/circuit.h"
#include "backend/circuit/circuitBlockDataManager.h"

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

	void resetStates() {}
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
	bool stepBack() { return false; }
	void stepForward() {}
	bool skipBack() { return false; }
	bool skipForward() { return false; }
	bool isViewingReplay() const { return false; }
	void setRealistic(bool realistic) { evalConfig.setRealistic(realistic); }
	bool isRealistic() const { return evalConfig.isRealistic(); }
	void setTickrate(double tickrate) { evalConfig.setTargetTickrate(tickrate); }
	double getTickrate() const { return evalConfig.getTargetTickrate(); }
	void increaseTickrateSeq() {}
	void decreaseTickrateSeq() {}
	void setUseTickrate(bool useTickrate) { evalConfig.setTickrateLimiter(useTickrate); }
	bool getUseTickrate() const { return evalConfig.isTickrateLimiterEnabled(); }
	double getRealTickrate() const { return 0; }
	void makeEdit(DifferenceSharedPtr difference, circuit_id_t circuitId);
	logic_state_t getState(const Address& address) { return logic_state_t::LOW; }
	void setState(const Address& address, logic_state_t state) {}
	circuit_id_t getCircuitId() const { return circuitId; }
	circuit_id_t getCircuitId(const Address& address) const { if (address.size() == 0) return circuitId; return 0; }
	// const EvalAddressTree buildAddressTree() const;
	// const EvalAddressTree buildAddressTree(eval_circuit_id_t evalCircuitId) const;

	simulator_id_t getBlockSimulatorId(const Address& address) const { return 0; }
	std::variant<simulator_id_t, std::vector<simulator_id_t>> getPinSimulatorId(const Address& address) const { return 0; }

	std::vector<simulator_id_t> getBlockSimulatorIds(const Address& addressOrigin, const std::vector<Position>& positions) const{ return {0}; }
	std::vector<std::variant<simulator_id_t, std::vector<simulator_id_t>>> getPinSimulatorIds(const Address& addressOrigin, const std::vector<Position>& positions) const { return {0}; }

	logic_state_t getStateFromSimulatorId(simulator_id_t simulatorId) const { return logic_state_t::UNDEFINED; }
	std::vector<logic_state_t> getStatesFromSimulatorIds(const std::vector<simulator_id_t>& simulatorIds) const { return {logic_state_t::UNDEFINED}; }

	void connectListener(
		void* object,
		const Address& address,
		SimulatorMappingUpdateListenerFunction func
	) {}
	void disconnectListener(void* object) {
		auto iter = listeners.find(object);
		if (iter != listeners.end()) {
			listeners.erase(iter);
		}
	}

	nlohmann::json dumpState() const;

private:
	circuit_id_t circuitId;
	evaluator_id_t evaluatorId;
	CircuitManager& circuitManager;
	BlockDataManager& blockDataManager;
	CircuitBlockDataManager& circuitBlockDataManager;
	DataUpdateEventManager& dataUpdateEventManager;
	DataUpdateEventManager::DataUpdateEventReceiver receiver;
	EvalConfig evalConfig;
	std::map<void*, SimulatorMappingUpdateListener> listeners;
};

#endif /* evaluator_h */
