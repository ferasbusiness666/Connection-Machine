#include "layerRunner.h"

#include "evalLayerState.h"
#include "passThroughEvalLayer.h"
// #include "junctionAddEvalLayer.h"
#include "junctionMergeEvalLayer.h"

LayerRunner::LayerRunner(const BlockDataManager& blockDataManager) : blockDataManager(blockDataManager) {
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	// layers.emplace_back(std::make_unique<JunctionAddEvalLayer>());
	layers.emplace_back(std::make_unique<JunctionMergeEvalLayer>());
	layers.emplace_back(std::make_unique<PassThroughEvalLayer>());
	evalTopLayerState = std::make_unique<EvalLayerState>(blockDataManager);
	assert(evalTopLayerState);
}

LayerRunner::~LayerRunner() = default;

void LayerRunner::runAll() {
	// logInfo("----------------");
	EvalLayerState* last = evalTopLayerState.get();
	for (unsigned int i = 0; i < layers.size(); i++) {
		EvalLayerState& next = last->getOrMakeNextLayerState();
		next.resetEdits();
		// logInfo("Running layer with {}, {}, {}, {}", "",
		// 	std::distance(last->getRemovedConnectionsBegin(), last->getRemovedConnectionsEnd()),
		// 	std::distance(last->getRemovedGatesBegin(), last->getRemovedGatesEnd()),
		// 	std::distance(last->getAddedGatesBegin(), last->getAddedGatesEnd()),
		// 	std::distance(last->getAddedConnectionsBegin(), last->getAddedConnectionsEnd())
		// );
		layers[i]->run(*last, next);
		last = &next;
	}
}

EvalLayerState& LayerRunner::getInputLayer() {
	return *evalTopLayerState;
}

const EvalLayerState& LayerRunner::getInputLayer() const {
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
	const EvalLayerState* layerState = evalTopLayerState.get()->getNextLayerState();
	for (unsigned int i = 1; true; i++) {
		auto connectionPointIter = layerState->getConnectionPointRemapping().find(evalConnectionPoint);
		if (connectionPointIter == layerState->getConnectionPointRemapping().end()) {
			auto evalGateIdIter = layerState->getEvalGateIdRemapping().find(evalConnectionPoint.gateId);
			if (evalGateIdIter == layerState->getEvalGateIdRemapping().end()) {
				logError("Could not find mapping for evalConnectionPoint.", "LayerRunner::getMappedEvalConnectionPoint");
				return EvalConnectionPoint(0, 0);
			}
			evalConnectionPoint.gateId = evalGateIdIter->second;
		} else {
			evalConnectionPoint = connectionPointIter->second;
		}
		if (i == layers.size()) break;
		layerState = layerState->getNextLayerState();
		assert(layerState);
	}
	return evalConnectionPoint;
}

std::vector<EvalConnectionPoint> LayerRunner::getReversedMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const {
	std::vector<EvalConnectionPoint> evalConnectionPoints = { evalConnectionPoint };
	std::vector<EvalConnectionPoint> lastEvalConnectionPoints;
	const EvalLayerState* layerState = &getOutputLayer();
	for (unsigned int i = 1; true; i++) {
		lastEvalConnectionPoints = std::move(evalConnectionPoints);
		evalConnectionPoints.clear();
		for (EvalConnectionPoint point : lastEvalConnectionPoints) {
			auto connectionPointIterPair = layerState->getConnectionPointReverseRemapping().equal_range(point);
			for (auto iter = connectionPointIterPair.first; iter != connectionPointIterPair.second; iter++) {
				evalConnectionPoints.push_back(iter->second);
			}
			auto evalGateIdIterPair = layerState->getEvalGateIdReverseRemapping().equal_range(point.gateId);
			for (auto iter = evalGateIdIterPair.first; iter != evalGateIdIterPair.second; iter++) {
				evalConnectionPoints.emplace_back(iter->second, evalConnectionPoint.connectionEndId);
			}
		}
		if (i == layers.size()) break;
		layerState = layerState->getLastLayerState();
		assert(layerState);
	}
	return evalConnectionPoints;
}
