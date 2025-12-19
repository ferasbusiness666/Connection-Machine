#ifndef passThroughEvalLayer_h
#define passThroughEvalLayer_h

#include "baseEvalLayer.h"

class EvalLayerState;

class PassThroughEvalLayer : public BaseEvalLayer {
public:
	void run(const EvalLayerState& currentState, EvalLayerState& nextState) override final;
};

#endif /* passThroughEvalLayer_h */
