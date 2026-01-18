#include "layerRunner.h"

#include "evalLayerState.h"
#include "junctionAddEvalLayer.h"
#include "junctionMergeEvalLayer.h"
#include "subcircuitEvalLayer.h"
#include "switchReplacerEvalLayer.h"

LayerRunner::LayerRunner(IdProvider<eval_gate_id>& evalGateIdProvider, Evaluator& evaluator, const CircuitManager& circuitManager) {
	evalTopLayerState = std::make_unique<EvalLayerState>(evalGateIdProvider);
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
	for (int i = 0; i < layers.size(); i++) {
		evalConnectionPoint = layers[i]->getMappedEvalConnectionPoint(evalConnectionPoint);
	}
	return evalConnectionPoint;
}

std::vector<EvalConnectionPoint> LayerRunner::getReversedMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const {
	std::vector<EvalConnectionPoint> evalConnectionPoints = { evalConnectionPoint };
	std::vector<EvalConnectionPoint> lastSimulatorConnectionPoints;
	for (int i = layers.size() - 1; i >= 0; i--) {
		lastSimulatorConnectionPoints = std::move(evalConnectionPoints);
		evalConnectionPoints.clear();
		for (EvalConnectionPoint point : lastSimulatorConnectionPoints) {
			layers[i]->getReversedMappedEvalConnectionPoint(point, evalConnectionPoints);
		}
	}
	return evalConnectionPoints;
}
