#include "layerRunner.h"

#include "evalLayerState.h"
#include "junctionAddEvalLayer.h"
#include "junctionMergeEvalLayer.h"
#include "subcircuitEvalLayer.h"
#include "switchReplacerEvalLayer.h"

LayerRunner::LayerRunner(Evaluator& evaluator, const CircuitManager& circuitManager) {
	evalTopLayerState = std::make_unique<EvalLayerState>();
	layers.emplace_back(std::make_unique<SubcircuitEvalLayer>(*evalTopLayerState, evaluator, circuitManager));
	layers.emplace_back(std::make_unique<SwitchReplacerEvalLayer>(layers.back()->getNextState()));
	layers.emplace_back(std::make_unique<JunctionAddEvalLayer>(layers.back()->getNextState()));
	layers.emplace_back(std::make_unique<JunctionMergeEvalLayer>(layers.back()->getNextState()));
	assert(evalTopLayerState);
}

LayerRunner::~LayerRunner() = default;

void LayerRunner::runAll() {
	// logInfo("------------------------------------------------");
	// evalTopLayerState->visualize();
	for (std::unique_ptr<BaseEvalLayer>& layer : layers) {
		// logInfo("----");
		// layer->getNextState().visualize();
		layer->run();
		// layer->getNextState().visualize();
	}
}

void LayerRunner::resetEdits() {
	evalTopLayerState->resetEdits();
	for (std::unique_ptr<BaseEvalLayer>& layer : layers) {
		layer->getNextState().resetEdits();
	}
}

EvalLayerState& LayerRunner::getInputLayer() { return *evalTopLayerState; }

const EvalLayerState& LayerRunner::getInputLayer() const { return *evalTopLayerState; }

const EvalLayerState& LayerRunner::getOutputLayer() const { return layers.back()->getNextState(); }

EvalConnectionPoint LayerRunner::getMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const {
	const EvalLayerState* layerState = evalTopLayerState.get()->getNextLayerState();
	for (unsigned int i = 1; i <= layers.size(); i++) {
		auto connectionPointIter = layerState->getConnectionPointRemapping().find(evalConnectionPoint);
		if (connectionPointIter == layerState->getConnectionPointRemapping().end()) {
			auto evalGateIdIter = layerState->getGateIdRemapping().find(evalConnectionPoint.gateId);
			if (evalGateIdIter == layerState->getGateIdRemapping().end()) {
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
	std::vector<EvalConnectionPoint> lastSimulatorConnectionPoints;
	const EvalLayerState* layerState = &getOutputLayer();
	for (unsigned int i = 1; i <= layers.size(); i++) {
		lastSimulatorConnectionPoints = std::move(evalConnectionPoints);
		evalConnectionPoints.clear();
		for (EvalConnectionPoint point : lastSimulatorConnectionPoints) {
			auto connectionPointIterPair = layerState->getConnectionPointReverseRemapping().equal_range(point);
			for (auto iter = connectionPointIterPair.first; iter != connectionPointIterPair.second; iter++) {
				evalConnectionPoints.push_back(iter->second);
			}
			auto evalGateIdIterPair = layerState->getGateIdReverseRemapping().equal_range(point.gateId);
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
