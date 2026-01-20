#include "layerRunner.h"

#include "evalLayerState.h"
#include "subcircuitEvalLayer.h"
#include "junctionAddEvalLayer.h"
#include "junctionMergeEvalLayer.h"
#include "switchReplacerEvalLayer.h"
#include "busReplacerEvalLayer.h"

LayerRunner::LayerRunner(IdProvider<eval_gate_id>& evalGateIdProvider, Evaluator& evaluator, const CircuitManager& circuitManager) {
	evalTopLayerState = std::make_unique<EvalLayerState>(evalGateIdProvider);
	layers.emplace_back(std::make_unique<SubcircuitEvalLayer>(*evalTopLayerState, evaluator, circuitManager));
	layers.emplace_back(std::make_unique<SwitchReplacerEvalLayer>(layers.back()->getNextState(), circuitManager));
	layers.emplace_back(std::make_unique<JunctionAddEvalLayer>(layers.back()->getNextState(), circuitManager));
	layers.emplace_back(std::make_unique<JunctionMergeEvalLayer>(layers.back()->getNextState(), circuitManager));
	layers.emplace_back(std::make_unique<BusReplacerEvalLayer>(layers.back()->getNextState(), circuitManager));
	layers.emplace_back(std::make_unique<JunctionMergeEvalLayer>(layers.back()->getNextState(), circuitManager));
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
const EvalLayerState& LayerRunner::getOutputLayerForOtherEvals() const { return layers[layers.size() - 3]->getNextState(); }

std::vector<std::pair<eval_gate_id, circuit_id_t>> LayerRunner::getSubcircuits() const {
	return dynamic_cast<SubcircuitEvalLayer*>(layers[0].get())->getSubcircuits();
}

EvalConnectionPoint LayerRunner::getMappedAddress(eval_gate_id gateId, const Address& address) const {
	EvalConnectionPoint evalConnectionPoint = dynamic_cast<SubcircuitEvalLayer*>(layers[0].get())->getMappedAddress(gateId, address);
	if (evalConnectionPoint.isNull()) return EvalConnectionPoint::null();
	for (unsigned int i = 1; i < layers.size(); i++) {
		evalConnectionPoint = layers[i]->getMappedEvalConnectionPoint(evalConnectionPoint);
	}
	return evalConnectionPoint;
}

EvalConnectionPoint LayerRunner::getMappedAddressForOtherEvals(eval_gate_id gateId, const Address& address) const {
	EvalConnectionPoint evalConnectionPoint = dynamic_cast<SubcircuitEvalLayer*>(layers[0].get())->getMappedAddress(gateId, address);
	if (evalConnectionPoint.isNull()) return EvalConnectionPoint::null();
	for (unsigned int i = 1; i < layers.size() - 2; i++) { // skip bus stuff
		evalConnectionPoint = layers[i]->getMappedEvalConnectionPoint(evalConnectionPoint);
	}
	return evalConnectionPoint;
}

VecEvalConnectionPoint LayerRunner::getReversedMappedConnectionPointWithAddress(EvalConnectionPoint evalConnectionPoint, eval_gate_id gateId, const Address& address) const {
	VecEvalConnectionPoint evalConnectionPoints = { evalConnectionPoint };
	VecEvalConnectionPoint lastSimulatorConnectionPoints;
	for (unsigned int i = layers.size() - 1; i >= 1; i--) {
		lastSimulatorConnectionPoints = std::move(evalConnectionPoints);
		evalConnectionPoints.clear();
		for (EvalConnectionPoint point : lastSimulatorConnectionPoints) {
			layers[i]->getReversedMappedEvalConnectionPoint(point, evalConnectionPoints);
		}
	}
	return dynamic_cast<SubcircuitEvalLayer*>(layers[0].get())->getReversedMappedConnectionPointsWithAddressMixed(evalConnectionPoints, gateId, address);
}

VecVecEvalConnectionPoint LayerRunner::getReversedMappedConnectionPointsWithAddress(const VecEvalConnectionPoint& evalConnectionPoints, eval_gate_id gateId, const Address& address) const {
	VecVecEvalConnectionPoint evalConnectionPointVecs;
	evalConnectionPointVecs.clear();
	for (EvalConnectionPoint connectionPoint : evalConnectionPoints) {
		evalConnectionPointVecs.push_back({});
		layers.back()->getReversedMappedEvalConnectionPoint(connectionPoint, evalConnectionPointVecs.back());
	}
	VecVecEvalConnectionPoint lastSimulatorConnectionPointVecs;
	for (unsigned int i = layers.size() - 2; i >= 1; i--) {
		lastSimulatorConnectionPointVecs = std::move(evalConnectionPointVecs);
		evalConnectionPointVecs.clear();
		for (VecEvalConnectionPoint vec : lastSimulatorConnectionPointVecs) {
			evalConnectionPointVecs.push_back({});
			for (EvalConnectionPoint connectionPoint : vec) {
				layers[i]->getReversedMappedEvalConnectionPoint(connectionPoint, evalConnectionPointVecs.back());
			}
		}
	}
	return dynamic_cast<SubcircuitEvalLayer*>(layers[0].get())->getReversedMappedConnectionPointGroupsWithAddress(evalConnectionPointVecs, gateId, address);
}
VecEvalConnectionPoint LayerRunner::getReversedMappedConnectionPointsWithAddressMixedForOtherEvals(VecEvalConnectionPoint evalConnectionPoints, eval_gate_id gateId, const Address& address) const {
	// VecEvalConnectionPoint evalConnectionPoints;
	VecEvalConnectionPoint lastSimulatorConnectionPoints;
	for (unsigned int i = layers.size() - 3; i >= 1; i--) { // skip bus stuff
		lastSimulatorConnectionPoints = std::move(evalConnectionPoints);
		evalConnectionPoints.clear();
		for (EvalConnectionPoint connectionPoint : lastSimulatorConnectionPoints) {
			layers[i]->getReversedMappedEvalConnectionPoint(connectionPoint, evalConnectionPoints);
		}
	}
	return dynamic_cast<SubcircuitEvalLayer*>(layers[0].get())->getReversedMappedConnectionPointsWithAddressMixed(evalConnectionPoints, gateId, address);
}
VecVecEvalConnectionPoint LayerRunner::getReversedMappedConnectionPointGroupsWithAddress(VecVecEvalConnectionPoint evalConnectionPoints, eval_gate_id gateId, const Address& address) const {
	VecVecEvalConnectionPoint lastSimulatorConnectionPoints;
	for (unsigned int i = layers.size() - 1; i >= 1; i--) {
		lastSimulatorConnectionPoints = std::move(evalConnectionPoints);
		evalConnectionPoints.clear();
		for (VecEvalConnectionPoint vec : lastSimulatorConnectionPoints) {
			evalConnectionPoints.push_back({});
			for (EvalConnectionPoint connectionPoint : vec) {
				layers[i]->getReversedMappedEvalConnectionPoint(connectionPoint, evalConnectionPoints.back());
			}
		}
	}
	return dynamic_cast<SubcircuitEvalLayer*>(layers[0].get())->getReversedMappedConnectionPointGroupsWithAddress(evalConnectionPoints, gateId, address);
}
VecVecEvalConnectionPoint LayerRunner::getReversedMappedConnectionPointGroupsWithAddressForOtherEvals(VecVecEvalConnectionPoint evalConnectionPoints, eval_gate_id gateId, const Address& address) const {
	VecVecEvalConnectionPoint lastSimulatorConnectionPoints;
	for (unsigned int i = layers.size() - 3; i >= 1; i--) {
		lastSimulatorConnectionPoints = std::move(evalConnectionPoints);
		evalConnectionPoints.clear();
		for (VecEvalConnectionPoint vec : lastSimulatorConnectionPoints) {
			evalConnectionPoints.push_back({});
			for (EvalConnectionPoint connectionPoint : vec) {
				layers[i]->getReversedMappedEvalConnectionPoint(connectionPoint, evalConnectionPoints.back());
			}
		}
	}
	return dynamic_cast<SubcircuitEvalLayer*>(layers[0].get())->getReversedMappedConnectionPointGroupsWithAddress(evalConnectionPoints, gateId, address);
}

EvalConnectionPoint LayerRunner::getMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const {
	for (unsigned int i = 0; i < layers.size(); i++) {
		evalConnectionPoint = layers[i]->getMappedEvalConnectionPoint(evalConnectionPoint);
	}
	return evalConnectionPoint;
}

EvalConnectionPoint LayerRunner::getMappedEvalConnectionPointForOtherEvals(EvalConnectionPoint evalConnectionPoint) const {
	for (unsigned int i = 0; i < layers.size() - 2; i++) {
		evalConnectionPoint = layers[i]->getMappedEvalConnectionPoint(evalConnectionPoint);
	}
	return evalConnectionPoint;
}

VecEvalConnectionPoint LayerRunner::getReversedMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const {
	VecEvalConnectionPoint evalConnectionPoints = { evalConnectionPoint };
	VecEvalConnectionPoint lastSimulatorConnectionPoints;
	for (int i = layers.size() - 1; i >= 0; i--) {
		lastSimulatorConnectionPoints = std::move(evalConnectionPoints);
		evalConnectionPoints.clear();
		for (EvalConnectionPoint point : lastSimulatorConnectionPoints) {
			layers[i]->getReversedMappedEvalConnectionPoint(point, evalConnectionPoints);
		}
	}
	return evalConnectionPoints;
}

void LayerRunner::getReversedMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint, VecEvalConnectionPoint& outputVector) const {
	VecEvalConnectionPoint evalConnectionPoints = { evalConnectionPoint };
	VecEvalConnectionPoint lastSimulatorConnectionPoints;
	for (unsigned int i = layers.size() - 1; i >= 1; i--) {
		lastSimulatorConnectionPoints = std::move(evalConnectionPoints);
		evalConnectionPoints.clear();
		for (EvalConnectionPoint point : lastSimulatorConnectionPoints) {
			layers[i]->getReversedMappedEvalConnectionPoint(point, evalConnectionPoints);
		}
	}
	for (EvalConnectionPoint point : evalConnectionPoints) {
		layers[0]->getReversedMappedEvalConnectionPoint(point, outputVector);
	}
}

void LayerRunner::getReversedMappedEvalConnectionPointForOtherEvals(EvalConnectionPoint evalConnectionPoint, VecEvalConnectionPoint& outputVector) const {
	VecEvalConnectionPoint evalConnectionPoints = { evalConnectionPoint };
	VecEvalConnectionPoint lastSimulatorConnectionPoints;
	for (unsigned int i = layers.size() - 3; i >= 1; i--) {
		lastSimulatorConnectionPoints = std::move(evalConnectionPoints);
		evalConnectionPoints.clear();
		for (EvalConnectionPoint point : lastSimulatorConnectionPoints) {
			layers[i]->getReversedMappedEvalConnectionPoint(point, evalConnectionPoints);
		}
	}
	for (EvalConnectionPoint point : evalConnectionPoints) {
		layers[0]->getReversedMappedEvalConnectionPoint(point, outputVector);
	}
}
