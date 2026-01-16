#ifndef switchReplacerEvalLayer_h
#define switchReplacerEvalLayer_h

#include "baseEvalLayer.h"

class SwitchReplacerEvalLayer : public BaseEvalLayer {
public:
	void run(const EvalLayerState& currentState, EvalLayerState& nextState) override final;
};

#endif /* switchReplacerEvalLayer_h */
