#include "portAdder.h"
#include "../../events/customEvents.h"

void PortAdder::activate() {
	CircuitTool::activate();
	registerFunction("Tool Primary Activate", std::bind(&PortAdder::press, this, std::placeholders::_1));
	setStatusBar("Left click to add the port at that location.");
}

bool PortAdder::press(const Event* event) {
	if (type == BlockType::NONE || !circuit || circuit->getBlockType() != type || !circuitView) return false;
	const PositionEvent* positionEvent = event->cast<PositionEvent>();
	if (!positionEvent) return false;
	bool can = true;
	if (!circuit->getBlockContainer().checkCollision(lastPointerPosition)) {
		can = false;
	} else if (isInput) {
		if (circuit->getBlockContainer().getInputOrBidirectionalConnectionEnd(lastPointerPosition).has_value()) {
			can = false;
		}
	} else {
		if (circuit->getBlockContainer().getOutputOrBidirectionalConnectionEnd(lastPointerPosition).has_value()) {
			can = false;
		}
	}
	if (can) onSelectFunction(positionEvent->getPosition());
	reset();
	toolStackInterface->switchToStack(0);
	return true;
}

void PortAdder::updateElements() {
	elementCreator.clear();
	bool can = true;
	if (!circuit->getBlockContainer().checkCollision(lastPointerPosition)) {
		can = false;
	} else if (isInput) {
		if (circuit->getBlockContainer().getInputOrBidirectionalConnectionEnd(lastPointerPosition).has_value()) {
			can = false;
		}
	} else {
		if (circuit->getBlockContainer().getOutputOrBidirectionalConnectionEnd(lastPointerPosition).has_value()) {
			can = false;
		}
	}
	elementCreator.addSelectionElement(SelectionElement(lastPointerPosition, !can));
}
