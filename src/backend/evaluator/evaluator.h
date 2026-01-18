#ifndef evaluator_h
#define evaluator_h

#include "backend/circuit/circuitDefs.h"
#include "backend/container/block/blockDefs.h"
#include "backend/container/block/connectionEnd.h"
#include "backend/dataUpdateEventManager.h"
#include "backend/position/position.h"

class EvaluatorInternal;
class DataUpdateEventManager;
class CircuitManager;
class EvalLogicSimulator;
class SubcircuitEvalLayer;
class Difference;
typedef std::shared_ptr<Difference> DifferenceSharedPtr;

class Evaluator {
	friend SubcircuitEvalLayer;
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
	void doLayersUpdate(bool doStartEdit = true);

	circuit_id_t getCircuitId() const;

	std::vector<std::pair<Position, circuit_id_t>> getSubcircuits() const;

	nlohmann::json dumpState() const;

	void addSimulator(EvalLogicSimulator& simulator) const { simulatorsUsingThisEvaluator.insert(&simulator); }
	void removeSimulator(EvalLogicSimulator& simulator) const { simulatorsUsingThisEvaluator.erase(&simulator); }

private:
	void startEdit();
	void endEdit();

	const Circuit& circuit;
	CircuitManager& circuitManager;
	DataUpdateEventManager& dataUpdateEventManager;
	DataUpdateEventManager::DataUpdateEventReceiver receiver;
	std::unique_ptr<EvaluatorInternal> evaluatorInternal;
	mutable std::unordered_set<EvalLogicSimulator*> simulatorsUsingThisEvaluator;
};

#endif /* evaluator_h */
