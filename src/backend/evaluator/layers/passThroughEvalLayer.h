#ifndef passThrough_h
#define passThrough_h

class EvalLayerState;

class PassThroughEvalLayer {
public:
	void run(const EvalLayerState& before, EvalLayerState& after);
};

#endif /* passThrough_h */
