#ifndef junctionAddEvalLayer_h
#define junctionAddEvalLayer_h

#include "baseEvalLayer.h"

class JunctionAddEvalLayer : public BaseEvalLayer {
public:
	JunctionAddEvalLayer(EvalLayerState& currentState, const CircuitManager& circuitManager) : BaseEvalLayer(currentState, circuitManager) {}
	void run() override final;
};

#endif /* junctionAddEvalLayer_h */
