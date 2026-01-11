#ifndef junctionAddEvalLayer_h
#define junctionAddEvalLayer_h

#include "baseEvalLayer.h"

class JunctionAddEvalLayer : public BaseEvalLayer {
public:
	void run(const EvalLayerState& currentState, EvalLayerState& nextState) override final;
};

#endif /* junctionAddEvalLayer_h */
