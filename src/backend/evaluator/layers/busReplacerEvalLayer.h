#ifndef busReplacerEvalLayer_h
#define busReplacerEvalLayer_h

#include "baseEvalLayer.h"

class BusReplacerEvalLayer : public BaseEvalLayer {
public:
	BusReplacerEvalLayer(EvalLayerState& currentState) : BaseEvalLayer(currentState) {}
	void run() override final;
};

#endif /* busReplacerEvalLayer_h */
