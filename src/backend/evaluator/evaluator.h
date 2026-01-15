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
class EvalLogicSimulator;
class SubcircuitEvalLayer;
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

	EvaluatorInternal& getEvaluatorInternal() { return *evaluatorInternal; }
	const EvaluatorInternal& getEvaluatorInternal() const { return *evaluatorInternal; }

	void makeEdit(DifferenceSharedPtr difference);

	circuit_id_t getCircuitId() const;
	circuit_id_t getCircuitId(const Address& address) const;

	nlohmann::json dumpState() const;

	void addSimulator(EvalLogicSimulator& simulator) const { simulatorsUsingThisEvaluator.insert(&simulator); }
	void removeSimulator(EvalLogicSimulator& simulator) const { simulatorsUsingThisEvaluator.erase(&simulator); }
	void addEvaluator(SubcircuitEvalLayer& evaluator) const { evaluatorsUsingThisEvaluator[&evaluator] += 1; }
	void removeEvaluator(SubcircuitEvalLayer& evaluator) const {
		auto iter = evaluatorsUsingThisEvaluator.find(&evaluator);
		assert(iter != evaluatorsUsingThisEvaluator.end());
		if (iter->second == 1) evaluatorsUsingThisEvaluator.erase(iter);
		else iter->second -= 1;
	}

private:
	const Circuit& circuit;
	CircuitManager& circuitManager;
	DataUpdateEventManager& dataUpdateEventManager;
	DataUpdateEventManager::DataUpdateEventReceiver receiver;
	std::unique_ptr<EvaluatorInternal> evaluatorInternal;
	mutable std::unordered_set<EvalLogicSimulator*> simulatorsUsingThisEvaluator;
	mutable std::unordered_map<SubcircuitEvalLayer*, unsigned int> evaluatorsUsingThisEvaluator;
};

#endif /* evaluator_h */
