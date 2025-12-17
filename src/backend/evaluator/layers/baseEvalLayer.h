#ifndef baseEvalLayer_h
#define baseEvalLayer_h

class EvalLayerState;

class BaseEvalLayer {
public:
	virtual ~BaseEvalLayer() = 0;
	virtual void run(const EvalLayerState& before, EvalLayerState& after) = 0;
};

#endif /* baseEvalLayer_h */
