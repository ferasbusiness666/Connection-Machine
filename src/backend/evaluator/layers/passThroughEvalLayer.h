#ifndef passThrough_h
#define passThrough_h

class EvalLayerState;

namespace PassThroughEvalLayer {
	void run(const EvalLayerState& before, EvalLayerState& after);
};

#endif /* passThrough_h */
