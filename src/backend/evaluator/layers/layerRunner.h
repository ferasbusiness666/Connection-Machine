#ifndef layerRunner_h
#define layerRunner_h

class EvalLayerState;
class BaseEvalLayer;

class LayerRunner {
public:
	LayerRunner();
	~LayerRunner();
	void runAll();
	EvalLayerState& getInputLayer();
	const EvalLayerState& getOutputLayer() const;
private:
	std::unique_ptr<EvalLayerState> evalTopLayerState;
	std::vector<std::unique_ptr<BaseEvalLayer>> layers;
};

#endif /* layerRunner_h */