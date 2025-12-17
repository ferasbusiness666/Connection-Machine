#ifndef layerRunner_h
#define layerRunner_h

class EvalLayerState;
class BaseEvalLayer;

class LayerRunner {
public:
	LayerRunner();
	void runAll(const EvalLayerState& before, EvalLayerState& after);
private:
	std::vector<EvalLayerState> evalLayerStates;
	std::vector<std::unique_ptr<BaseEvalLayer>> layers;
};

#endif /* layerRunner_h */