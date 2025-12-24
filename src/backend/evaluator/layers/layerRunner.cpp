#include "layerRunner.h"

#include "../util/evalLayerState.h"
#include "passThroughEvalLayer.h"

LayerRunner::LayerRunner() {
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	evalTopLayerState = std::make_unique<EvalLayerState>();
	assert(evalTopLayerState);
}

LayerRunner::~LayerRunner() = default;

void LayerRunner::runAll() {
	EvalLayerState* last = evalTopLayerState.get();
	for (unsigned int i = 0; i < layers.size(); i++) {
		EvalLayerState& next = last->getNextLayerState();
		next.resetEdits();
		layers[i]->run(*last, next);
		last = &next;
	}
}

EvalLayerState& LayerRunner::getInputLayer() {
	return *evalTopLayerState;
}

const EvalLayerState& LayerRunner::getOutputLayer() const {
	const EvalLayerState* last = evalTopLayerState.get();
	for (unsigned int i = 0; i < layers.size(); i++) {
		last = last->getNextLayerState();
		assert(last);
	}
	return *last;
}
