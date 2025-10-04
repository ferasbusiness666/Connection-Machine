#include "singleConnectTool.h"
#include "gui/viewportManager/circuitView/renderer/logicRenderingUtils.h"

bool SingleConnectTool::makeConnection(const Event* event) {
	if (!circuit) return false;

	if (clicked) {
		if (!circuit->tryRemoveConnection(clickPosition, lastPointerPosition)) {
			circuit->tryCreateConnection(clickPosition, lastPointerPosition);
		}
		reset();
		updateElements();
		return true;
	} else {
		if (!circuit->getBlockContainer()->getOutputConnectionEnd(lastPointerPosition).has_value()) {
			return false;
		}

		clicked = true;
		clickPosition = lastPointerPosition;
		updateElements();
		return true;
	}
}

bool SingleConnectTool::cancelConnection(const Event* event) {
	if (clicked) {
		reset();
		updateElements();
		return true;
	}
	return false;
}

void SingleConnectTool::updateElements() {
	if (!isActivate) return;

	if (elementCreator.isSetup()) {
		elementCreator.clear();

		if (!(circuit && pointerInView)) return;

		if (clicked) {
			const Block* outputBlock = circuit->getBlockContainer()->getBlock(clickPosition);
			if (!outputBlock) {
				reset();
				bool valid = circuit->getBlockContainer()->getOutputConnectionEnd(lastPointerPosition).has_value();
				elementCreator.addSelectionElement(SelectionElement(lastPointerPosition, !valid));
			} else {
				connection_end_id_t outputEndId = circuit->getBlockContainer()->getOutputConnectionEnd(clickPosition).value().getConnectionId();
				bool valid = circuit->getBlockContainer()->getInputConnectionEnd(lastPointerPosition).has_value();
				if (valid) {
					const Block* inputBlock = circuit->getBlockContainer()->getBlock(lastPointerPosition);
					connection_end_id_t inputEndId = circuit->getBlockContainer()->getInputConnectionEnd(lastPointerPosition).value().getConnectionId();
					elementCreator.addConnectionPreview(ConnectionPreview(
						clickPosition.free() + getOutputOffset(outputBlock->type(), outputEndId, outputBlock->getOrientation()),
						lastPointerPosition.free() + getInputOffset(inputBlock->type(), inputEndId, inputBlock->getOrientation())
					));
				} else {
					elementCreator.addHalfConnectionPreview(HalfConnectionPreview(
						clickPosition.free() + getOutputOffset(outputBlock->type(), outputEndId, outputBlock->getOrientation()),
						lastPointerFPosition
					));
				}
				elementCreator.addSelectionElement(SelectionElement(lastPointerPosition, !valid));
			}
		} else {
			// TODO - change to use isvalid function
			bool valid = circuit->getBlockContainer()->getOutputConnectionEnd(lastPointerPosition).has_value();
			elementCreator.addSelectionElement(SelectionElement(lastPointerPosition, !valid));
		}
	}

	if (clicked) {
		setStatusBar("Left click to set the connection end. Remake a connection to remove it.");
	} else {
		setStatusBar("Left click to set the connection start. Remake a connection to remove it.");
	}
}
