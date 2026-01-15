#ifndef simulatorManager_h
#define simulatorManager_h

#include "backend/circuit/circuit.h"
#include "simulator/simulatorDefs.h"

class CircuitManager;
class DataUpdateEventManager;
class EvalLogicSimulator;

class SimulatorManager {
public:
	SimulatorManager(CircuitManager& circuitManager, DataUpdateEventManager& dataUpdateEventManager);
	SimulatorManager(const SimulatorManager&) = delete;
    SimulatorManager& operator=(const SimulatorManager&) = delete;
	~SimulatorManager();

	inline const std::map<simulator_id_t, EvalLogicSimulator>& getSimulators() const { return simulators; }

	EvalLogicSimulator* getSimulator(simulator_id_t id);
	const EvalLogicSimulator* getSimulator(simulator_id_t id) const;

	simulator_id_t createNewSimulator(circuit_id_t circuitId);
	void destroySimulator(simulator_id_t id);

	typedef std::map<simulator_id_t, EvalLogicSimulator>::iterator iterator;
	typedef std::map<simulator_id_t, EvalLogicSimulator>::const_iterator const_iterator;

	inline iterator begin() { return simulators.begin(); }
	inline iterator end() { return simulators.end(); }
	inline const_iterator begin() const { return simulators.begin(); }
	inline const_iterator end() const { return simulators.end(); }

	void applyDiff(DifferenceSharedPtr difference, circuit_id_t circuitId);

	nlohmann::json dumpState() const;

private:
	IdProvider<simulator_id_t> simulatorIdProvider;

	DataUpdateEventManager& dataUpdateEventManager;
	CircuitManager& circuitManager;

	std::map<simulator_id_t, EvalLogicSimulator> simulators;
};

#endif /* simulatorManager_h */
