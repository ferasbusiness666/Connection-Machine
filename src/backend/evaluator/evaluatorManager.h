#ifndef evaluatorManager_h
#define evaluatorManager_h

#include "backend/circuit/circuit.h"
#include "evalDefs.h"

class CircuitManager;
class DataUpdateEventManager;

class EvaluatorManager {
public:
	EvaluatorManager(DataUpdateEventManager& dataUpdateEventManager);
	EvaluatorManager(const EvaluatorManager&) = delete;
    EvaluatorManager& operator=(const EvaluatorManager&) = delete;
	~EvaluatorManager();

	inline const std::map<evaluator_id_t, Evaluator>& getEvaluators() const { return evaluators; }

	Evaluator* getEvaluator(evaluator_id_t id);
	const Evaluator* getEvaluator(evaluator_id_t id) const;

	evaluator_id_t createNewEvaluator(CircuitManager& circuitManager, circuit_id_t circuitId);
	void destroyEvaluator(evaluator_id_t id);

	typedef std::map<evaluator_id_t, Evaluator>::iterator iterator;
	typedef std::map<evaluator_id_t, Evaluator>::const_iterator const_iterator;

	inline iterator begin() { return evaluators.begin(); }
	inline iterator end() { return evaluators.end(); }
	inline const_iterator begin() const { return evaluators.begin(); }
	inline const_iterator end() const { return evaluators.end(); }

	void applyDiff(DifferenceSharedPtr difference, circuit_id_t circuitId);

	nlohmann::json dumpState() const;

private:
	IdProvider<evaluator_id_t> evaluatorIdProvider;

	DataUpdateEventManager& dataUpdateEventManager;

	std::map<evaluator_id_t, Evaluator> evaluators;
};

#endif /* evaluatorManager_h */
