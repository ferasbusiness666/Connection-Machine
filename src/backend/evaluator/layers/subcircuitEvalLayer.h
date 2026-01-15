#ifndef subcircuitEvalLayer_h
#define subcircuitEvalLayer_h

#include "baseEvalLayer.h"

class CircuitManager;

class SubcircuitEvalLayer : public BaseEvalLayer {
	struct SubcircuitData {
		std::unordered_map<eval_gate_id, eval_gate_id> otherSimulatorToThissimulatorIdMapping;
	};
public:
	SubcircuitEvalLayer(const CircuitManager& circuitManager) : circuitManager(circuitManager) {}

	void run(const EvalLayerState& currentState, EvalLayerState& nextState) override final;

private:
	std::unordered_map<eval_gate_id, SubcircuitData> subcircuits;
	const CircuitManager& circuitManager;
};

#endif /* subcircuitEvalLayer_h */
