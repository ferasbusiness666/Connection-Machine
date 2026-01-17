#ifndef baseEvalLayer_h
#define baseEvalLayer_h

#include "backend/evaluator/evalDefs.h"

class EvalLayerState;

class BaseEvalLayer {
public:
	BaseEvalLayer(EvalLayerState& currentState);
	virtual ~BaseEvalLayer() = default;
	virtual void run() = 0;

	const EvalLayerState& getCurrentState() const { return currentState; };
	EvalLayerState& getNextState() { return nextState; };

protected:
	inline bool isJunctionType(EvalGateType gateType) const {
		return (
			gateType == getEvalGateType(BlockType::JUNCTION) || gateType == getEvalGateType(BlockType::JUNCTION_L) ||
			gateType == getEvalGateType(BlockType::JUNCTION_H) || gateType == getEvalGateType(BlockType::JUNCTION_X)
		);
	}

	const EvalLayerState& currentState;
	EvalLayerState& nextState;
};

#endif /* baseEvalLayer_h */
