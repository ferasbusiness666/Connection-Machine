#include "layerRunner.h"

#include "../util/evalLayerState.h"
#include "passThroughEvalLayer.h"

LayerRunner::LayerRunner() {
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	evalTopLayerState = std::make_unique<EvalLayerState>();
}

void LayerRunner::runAll() {
	EvalLayerState* last = evalTopLayerState.get();
	for (unsigned int i = 0; i < layers.size(); i++) {
		EvalLayerState& next = last->getNextLayerState();
		next.resetEdits();
		layers[i]->run(*last, next);
		last = &next;
	}
}
