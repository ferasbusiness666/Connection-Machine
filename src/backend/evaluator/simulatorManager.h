#ifndef evaluatorManager_h
#define evaluatorManager_h

#include "backend/circuit/circuit.h"
#include "evalDefs.h"

class CircuitManager;
class DataUpdateEventManager;
class Simulator {

};

class SimulatorManager {
public:
	SimulatorManager(DataUpdateEventManager& dataUpdateEventManager) : dataUpdateEventManager(dataUpdateEventManager), evaluatorIdProvider(1) {}
	SimulatorManager(const SimulatorManager&) = delete;
    SimulatorManager& operator=(const SimulatorManager&) = delete;

	inline const std::map<evaluator_id_t, Simulator>& getSimulator() const { return simulators; }

	inline Simulator* getSimulator(evaluator_id_t id) {
		auto iter = simulators.find(id);
		if (iter == simulators.end()) return nullptr;
		return &iter->second;
	}
	inline const Simulator* getSimulator(evaluator_id_t id) const {
		auto iter = simulators.find(id);
		if (iter == simulators.end()) return nullptr;
		return &iter->second;
	}

	evaluator_id_t createNewSimulator(CircuitManager& circuitManager, circuit_id_t circuitId);
	inline void destroySimulator(evaluator_id_t id) {
		auto iter = simulators.find(id);
		if (iter != simulators.end()) {
			evaluatorIdProvider.releaseId(id);
			simulators.erase(iter);
		}
	}

	typedef std::map<evaluator_id_t, Simulator>::iterator iterator;
	typedef std::map<evaluator_id_t, Simulator>::const_iterator const_iterator;

	inline iterator begin() { return simulators.begin(); }
	inline iterator end() { return simulators.end(); }
	inline const_iterator begin() const { return simulators.begin(); }
	inline const_iterator end() const { return simulators.end(); }

	void applyDiff(DifferenceSharedPtr difference, circuit_id_t circuitId);

	nlohmann::json dumpState() const;

private:
	IdProvider<evaluator_id_t> evaluatorIdProvider;

	DataUpdateEventManager& dataUpdateEventManager;

	std::map<evaluator_id_t, Simulator> simulators;
};

#endif /* evaluatorManager_h */
