#ifndef junctionMergeEvalLayer_h
#define junctionMergeEvalLayer_h

#include "backend/evaluator/evalDefs.h"
#include "baseEvalLayer.h"

class JunctionMergeEvalLayer : public BaseEvalLayer {
public:
	JunctionMergeEvalLayer(EvalLayerState& currentState) : BaseEvalLayer(currentState) {}
	void run() override final;

private:
	std::tuple<std::vector<eval_gate_id>, std::unordered_map<EvalConnectionPoint, unsigned int>, EvalGateType> gatherJunctionGroup(eval_gate_id gateId, const EvalLayerState& evalLayerState) const;
};

#endif /* junctionMergeEvalLayer_h */
