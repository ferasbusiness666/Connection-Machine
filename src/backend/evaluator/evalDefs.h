#ifndef evalDefs_h
#define evalDefs_h

#include "backend/position/position.h"
#include "util/id.h"

struct EvaluatorIdTag;
struct EvalCircuitIdTag;
struct MiddleIdTag;
struct ConnectionPortIdTag;
struct SimulatorIdTag;
using evaluator_id_t = Id<EvaluatorIdTag, unsigned int>;
using eval_circuit_id_t = Id<EvalCircuitIdTag, unsigned int>;
using middle_id_t = Id<MiddleIdTag, unsigned int>;
using simulator_id_t = Id<SimulatorIdTag, unsigned int>;

enum class SimulatorMappingUpdateType {
	BLOCK,
	PIN
};

struct SimulatorMappingUpdate {
	Position portPosition;
	std::variant<simulator_id_t, std::vector<simulator_id_t>> simulatorIds;
	SimulatorMappingUpdateType type;
};

typedef std::function<void(const std::vector<SimulatorMappingUpdate>&)> SimulatorMappingUpdateListenerFunction;

struct SimulatorMappingUpdateListener {
	eval_circuit_id_t evalCircuitId;
	std::function<void(const std::vector<SimulatorMappingUpdate>&)> callback;
};

class Evaluator;
typedef std::shared_ptr<class Evaluator> SharedEvaluator;

#endif /* evalDefs_h */
