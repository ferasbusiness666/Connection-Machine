#ifndef switchReplacerEvalLayer_h
#define switchReplacerEvalLayer_h

#include "baseEvalLayer.h"

class SwitchReplacerEvalLayer : public BaseEvalLayer {
public:
	SwitchReplacerEvalLayer(EvalLayerState& currentState) : BaseEvalLayer(currentState) {}
	void run() override final;
};

#endif /* switchReplacerEvalLayer_h */
