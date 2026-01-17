#ifndef layerRunner_h
#define layerRunner_h

#include "../evalDefs.h"

class CircuitManager;
class BlockDataManager;
class EvalLayerState;
class BaseEvalLayer;

class LayerRunner {
public:
	LayerRunner(Evaluator& evaluator, const CircuitManager& circuitManager);
	~LayerRunner();
	void runAll();
	void resetEdits();
	EvalLayerState& getInputLayer();
	const EvalLayerState& getInputLayer() const;
	const EvalLayerState& getOutputLayer() const;
	EvalConnectionPoint getMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const;
	std::vector<EvalConnectionPoint> getReversedMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const;
private:
	std::unique_ptr<EvalLayerState> evalTopLayerState;
	std::vector<std::unique_ptr<BaseEvalLayer>> layers;
};

#endif /* layerRunner_h */