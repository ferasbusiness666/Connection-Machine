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
class Difference;
typedef std::shared_ptr<Difference> DifferenceSharedPtr;

class Evaluator {
public:
	typedef std::tuple<BlockType, connection_end_id_t, Position> RemoveCircuitIOData; // I hate tuples, but this is how I get the data
	typedef std::pair<BlockType, connection_end_id_t> SetCircuitIOData;

	Evaluator(
		evaluator_id_t evaluatorId,
		CircuitManager& circuitManager,
		circuit_id_t circuitId,
		DataUpdateEventManager& dataUpdateEventManager
	);
	Evaluator(const Evaluator&) = delete;
    Evaluator& operator=(const Evaluator&) = delete;
	~Evaluator();

	inline evaluator_id_t getEvaluatorId() const { return evaluatorId; }
	std::string getEvaluatorName() const;

	void makeEdit(DifferenceSharedPtr difference, circuit_id_t circuitId);

	circuit_id_t getCircuitId() const { return circuitId; }
	circuit_id_t getCircuitId(const Address& address) const { if (address.size() == 0) return circuitId; return 0; }

	nlohmann::json dumpState() const;

	EvalLogicSimulator& getEvalLogicSimulator() { return evalLogicSimulator; }
	const EvalLogicSimulator& getEvalLogicSimulator() const { return evalLogicSimulator; }

private:
	circuit_id_t circuitId;
	evaluator_id_t evaluatorId;
	CircuitManager& circuitManager;
	DataUpdateEventManager& dataUpdateEventManager;
	DataUpdateEventManager::DataUpdateEventReceiver receiver;
	std::unique_ptr<EvaluatorInternal> evaluatorInternal;
	EvalLogicSimulator evalLogicSimulator;
};

#endif /* evaluator_h */
