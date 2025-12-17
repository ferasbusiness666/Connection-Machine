#include "layerRunner.h"

#include "../util/evalLayerState.h"
#include "passThroughEvalLayer.h"

LayerRunner::LayerRunner() {
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	evalLayerStates.resize(layers.size() - 1);
}

void LayerRunner::runAll(const EvalLayerState& before, EvalLayerState& after) {
	const EvalLayerState* last = &before;
	for (unsigned int i = 0; i < layers.size() - 1; i++) {
		evalLayerStates[i].resetEdits();
		layers[i]->run(*last, evalLayerStates[i]);
		last = &evalLayerStates[i];
	}
	after.resetEdits();
	layers[layers.size() - 1]->run(*last, after);
}
