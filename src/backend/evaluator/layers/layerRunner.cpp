#include "layerRunner.h"

#include "../util/evalLayerState.h"
#include "passThroughEvalLayer.h"

LayerRunner::LayerRunner(const BlockDataManager& blockDataManager) : blockDataManager(blockDataManager) {
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	evalTopLayerState = std::make_unique<EvalLayerState>(blockDataManager);
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

EvalConnectionPoint LayerRunner::getMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const {
	// const EvalLayerState* layerState = evalTopLayerState.get();
	// for (unsigned int i = 0; i < layers.size() + 1; i++) {
	// 	auto iter = layerState->getConnectionPointRemapping().find(evalConnectionPoint);
	// 	if (iter == layerState->getConnectionPointRemapping().end()) {
	// 		logError("layerState->getConnectionPointRemapping().find(evalConnectionPoint) failed.", "LayerRunner::getMappedEvalConnectionPoint");
	// 		return EvalConnectionPoint(0, 0);
	// 	}
	// 	evalConnectionPoint = iter->second;
	// 	if (i == layers.size()) break;
	// 	layerState = layerState->getNextLayerState();
	// 	assert(layerState);
	// }
	return evalConnectionPoint;
}

std::vector<EvalConnectionPoint> LayerRunner::getReversedMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const {
	std::vector<EvalConnectionPoint> evalConnectionPoints = { evalConnectionPoint };
	// std::vector<EvalConnectionPoint> lastEvalConnectionPoints;
	// const EvalLayerState* layerState = &getOutputLayer();
	// for (unsigned int i = 0; i < layers.size() + 1; i++) {
	// 	lastEvalConnectionPoints = std::move(evalConnectionPoints);
	// 	evalConnectionPoints.clear();
	// 	for (EvalConnectionPoint point : lastEvalConnectionPoints) {
	// 		auto iterPair = layerState->getConnectionPointReverseRemapping().equal_range(point);
	// 		for (auto iter = iterPair.first; iter != iterPair.second; iter++) {
	// 			evalConnectionPoints.push_back(iter->second);
	// 		}
	// 	}
	// 	if (i == layers.size()) break;
	// 	layerState = layerState->getNextLayerState();
	// 	assert(layerState);
	// }
	return evalConnectionPoints;
}
