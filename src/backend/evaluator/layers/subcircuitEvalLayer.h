#ifndef subcircuitEvalLayer_h
#define subcircuitEvalLayer_h

#include "backend/circuit/circuitDefs.h"
#include "baseEvalLayer.h"

class CircuitManager;
class EvalLayerState;

class SubcircuitEvalLayer : public BaseEvalLayer {
	struct SubcircuitData {
		SubcircuitData(circuit_id_t circuitId, const EvalLayerState& outputEvalLayer) : circuitId(circuitId), outputEvalLayer(outputEvalLayer) {}
		circuit_id_t circuitId;
		const EvalLayerState& outputEvalLayer;
		std::unordered_map<eval_gate_id, eval_gate_id> otherSimulatorToThisSimulatorIdMapping;
		std::unordered_map<eval_gate_id, eval_gate_id> thisSimulatorIdMappingToOtherSimulator;
	};
public:
	SubcircuitEvalLayer(const CircuitManager& circuitManager) : circuitManager(circuitManager) {}

	void run(const EvalLayerState& currentState, EvalLayerState& nextState) override final;

	// Only called by Evaluator
	void processEdits();

private:
	std::unordered_map<eval_gate_id, SubcircuitData> subcircuits;
	const CircuitManager& circuitManager;
};

#endif /* subcircuitEvalLayer_h */
