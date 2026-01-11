#ifndef baseEvalLayer_h
#define baseEvalLayer_h

#include "backend/evaluator/evalDefs.h"

class EvalLayerState;

class BaseEvalLayer {
public:
	virtual ~BaseEvalLayer() = default;
	virtual void run(const EvalLayerState& currentState, EvalLayerState& nextState) = 0;

protected:
	inline bool isJunctionType(EvalGateType gateType) const {
		return (
			gateType == getEvalGateType(BlockType::JUNCTION) || gateType == getEvalGateType(BlockType::JUNCTION_L) || gateType == getEvalGateType(BlockType::JUNCTION_H) ||
			gateType == getEvalGateType(BlockType::JUNCTION_X)
		);
	}
};

#endif /* baseEvalLayer_h */
