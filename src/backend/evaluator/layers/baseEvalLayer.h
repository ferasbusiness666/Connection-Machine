#ifndef baseEvalLayer_h
#define baseEvalLayer_h

class EvalLayerState;

class BaseEvalLayer {
public:
	virtual ~BaseEvalLayer() = default;
	virtual void run(const EvalLayerState& currentState, EvalLayerState& nextState) = 0;
};

#endif /* baseEvalLayer_h */
