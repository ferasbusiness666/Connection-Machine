#ifndef evaluator_h
#define evaluator_h

// #include "backend/blockData/blockData.h"
// #include "util/evalConfig.h"
// #include "simulator/logicState.h"
#include "backend/address.h"
#include "backend/circuit/circuitDefs.h"
#include "simulator/evalLogicSimulator.h"

class EvaluatorInternal;
class DataUpdateEventManager;
class CircuitManager;
class SimulatorManager;
class Difference;
typedef std::shared_ptr<Difference> DifferenceSharedPtr;

class Evaluator {
public:
	typedef std::tuple<BlockType, connection_end_id_t, Position> RemoveCircuitIOData; // I hate tuples, but this is how I get the data
	typedef std::pair<BlockType, connection_end_id_t> SetCircuitIOData;

	Evaluator(
		CircuitManager& circuitManager,
		const Circuit& circuit,
		DataUpdateEventManager& dataUpdateEventManager
	);
	Evaluator(const Evaluator&) = delete;
    Evaluator& operator=(const Evaluator&) = delete;
	~Evaluator();

	EvaluatorInternal& getSimulatorInternal() { return *evaluatorInternal; }
	const EvaluatorInternal& getSimulatorInternal() const { return *evaluatorInternal; }
	std::string getSimulatorName() const;

	void makeEdit(DifferenceSharedPtr difference);

	circuit_id_t getCircuitId() const;
	circuit_id_t getCircuitId(const Address& address) const;

	nlohmann::json dumpState() const;

	void addSimulator(EvalLogicSimulator& simulator) { simulatorsUsingThisEvaluator.insert(&simulator); }
	void removeSimulator(EvalLogicSimulator& simulator) { simulatorsUsingThisEvaluator.erase(&simulator); }

private:
	const Circuit& circuit;
	CircuitManager& circuitManager;
	DataUpdateEventManager& dataUpdateEventManager;
	DataUpdateEventManager::DataUpdateEventReceiver receiver;
	std::unique_ptr<EvaluatorInternal> evaluatorInternal;
	std::unordered_set<EvalLogicSimulator*> simulatorsUsingThisEvaluator;
};

#endif /* evaluator_h */
