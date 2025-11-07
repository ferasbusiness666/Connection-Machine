#include "circuitView.h"

#include "backend/backend.h"
#include "environment/environment.h"
#include "events/customEvents.h"

#include "gpu/mainRenderer.h"

CircuitView::CircuitView(Environment& environment, ViewportId viewportId) :
	backend(environment.getBackend()), dataUpdateEventManager(environment.getBackend().getDataUpdateEventManager()), viewportId(viewportId),
	toolManager(environment, &eventRegister, viewportId, this), tutorialManager(environment, *this) {
	MainRenderer::get().moveViewportView(viewportId, viewManager.getTopLeft(), viewManager.getBottomRight());
	viewManager.setUpEvents(eventRegister);
	viewManager.connectViewChanged(std::bind(&CircuitView::viewChanged, this));
}

void CircuitView::setEvaluator(evaluator_id_t evaluatorId, const Address& address) {
	if (evaluatorId == evaluator_id_t(0)) {
		this->evaluator = nullptr;
		this->circuit = nullptr;
		circuitRenderManager.reset();
		MainRenderer::get().setViewportEvaluator(viewportId, nullptr, Address());
		toolManager.setCircuit(nullptr);
		viewManager.setCircuit(nullptr);
		dataUpdateEventManager.sendEvent("circuitViewChangeEvaluator", this->evaluator);
		dataUpdateEventManager.sendEvent("circuitViewChangeCircuit", this->circuit);
	} else {
		SharedEvaluator evaluator = backend.getEvaluatorManager().getEvaluator(evaluatorId);
		if (evaluator == nullptr) {
			logError("When setting CircuitView's evaluator the wrong backend or evaluator id was passed. Failed to connect! Doing nothing!", "CircuitView");
		} else {
			this->evaluator = evaluator;
			this->address = address;
			circuit_id_t circuitId = evaluator->getCircuitId(address);
			SharedCircuit circuit = backend.getCircuit(circuitId); // ok if null
			this->circuit = circuit;
			circuitRenderManager.emplace(circuit.get(), viewportId);
			MainRenderer::get().setViewportEvaluator(viewportId, evaluator.get(), address);
			toolManager.setCircuit(circuit.get());
			viewManager.setCircuit(circuit.get());
			dataUpdateEventManager.sendEvent("circuitViewChangeEvaluator", this->evaluator);
			dataUpdateEventManager.sendEvent("circuitViewChangeCircuit", this->circuit);
		}
	}
}

void CircuitView::setEvaluator(std::shared_ptr<Evaluator> evaluator, const Address& address) {
	if (evaluator == nullptr) {
		this->evaluator = nullptr;
		this->circuit = nullptr;
		circuitRenderManager.reset();
		MainRenderer::get().setViewportEvaluator(viewportId, nullptr, Address());
		toolManager.setCircuit(nullptr);
		viewManager.setCircuit(nullptr);
		dataUpdateEventManager.sendEvent("circuitViewChangeEvaluator", this->evaluator);
		dataUpdateEventManager.sendEvent("circuitViewChangeCircuit", this->circuit);
	} else if (backend.getEvaluatorManager().getEvaluator(evaluator->getEvaluatorId()) != evaluator) {
		logError("When setting CircuitView's evaluator the wrong backend was passed. Failed to connect! Doing nothing!", "CircuitView");
	} else {
		this->evaluator = evaluator;
		this->address = address;
		circuit_id_t circuitId = evaluator->getCircuitId(address);
		SharedCircuit circuit = backend.getCircuit(circuitId); // ok if null
		this->circuit = circuit;
		circuitRenderManager.emplace(circuit.get(), viewportId);
		MainRenderer::get().setViewportEvaluator(viewportId, evaluator.get(), address);
		toolManager.setCircuit(circuit.get());
		viewManager.setCircuit(circuit.get());
		dataUpdateEventManager.sendEvent("circuitViewChangeEvaluator", this->evaluator);
		dataUpdateEventManager.sendEvent("circuitViewChangeCircuit", this->circuit);
	}
}

void CircuitView::setCircuit(circuit_id_t circuitId) {
	if (circuitId == 0) {
		this->evaluator = nullptr;
		this->circuit = nullptr;
		circuitRenderManager.reset();
		MainRenderer::get().setViewportEvaluator(viewportId, nullptr, Address());
		toolManager.setCircuit(nullptr);
		viewManager.setCircuit(nullptr);
		dataUpdateEventManager.sendEvent("circuitViewChangeEvaluator", this->evaluator);
		dataUpdateEventManager.sendEvent("circuitViewChangeCircuit", this->circuit);
	} else {
		SharedCircuit circuit = backend.getCircuit(circuitId);
		if (circuit == nullptr) {
			logError("When setting CircuitView's circuit the wrong backend or circuit id was passed. Failed to connect! Doing nothing!", "CircuitView");
		} else {
			this->evaluator = nullptr;
			this->circuit = circuit;
			circuitRenderManager.emplace(circuit.get(), viewportId);
			MainRenderer::get().setViewportEvaluator(viewportId, nullptr, Address());
			toolManager.setCircuit(circuit.get());
			viewManager.setCircuit(circuit.get());
			dataUpdateEventManager.sendEvent("circuitViewChangeEvaluator", this->evaluator);
			dataUpdateEventManager.sendEvent("circuitViewChangeCircuit", this->circuit);
		}
	}
}

void CircuitView::setCircuit(SharedCircuit circuit) {
	if (circuit == nullptr) {
		this->evaluator = nullptr;
		this->circuit = nullptr;
		circuitRenderManager.reset();
		MainRenderer::get().setViewportEvaluator(viewportId, nullptr, Address());
		toolManager.setCircuit(nullptr);
		viewManager.setCircuit(nullptr);
		dataUpdateEventManager.sendEvent("circuitViewChangeEvaluator", this->evaluator);
		dataUpdateEventManager.sendEvent("circuitViewChangeCircuit", this->circuit);
	} else if (backend.getCircuit(circuit->getCircuitId()) != circuit) {
		logError("When setting CircuitView's circuit the wrong backend was passed. Failed to connect! Doing nothing!", "CircuitView");
	} else {
		this->evaluator = nullptr;
		this->circuit = circuit;
		circuitRenderManager.emplace(circuit.get(), viewportId);
		MainRenderer::get().setViewportEvaluator(viewportId, nullptr, Address());
		toolManager.setCircuit(circuit.get());
		viewManager.setCircuit(circuit.get());
		dataUpdateEventManager.sendEvent("circuitViewChangeEvaluator", this->evaluator);
		dataUpdateEventManager.sendEvent("circuitViewChangeCircuit", this->circuit);
	}
}

void CircuitView::viewChanged() {
	eventRegister.doEvent(PositionEvent("Pointer Move", viewManager.getPointerPosition()));
	MainRenderer::get().moveViewportView(viewportId, viewManager.getTopLeft(), viewManager.getBottomRight());
}
