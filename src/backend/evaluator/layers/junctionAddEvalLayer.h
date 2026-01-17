#ifndef junctionAddEvalLayer_h
#define junctionAddEvalLayer_h

#include "baseEvalLayer.h"

class JunctionAddEvalLayer : public BaseEvalLayer {
public:
	JunctionAddEvalLayer(EvalLayerState& currentState) : BaseEvalLayer(currentState) {}
	void run() override final;
};

#endif /* junctionAddEvalLayer_h */
