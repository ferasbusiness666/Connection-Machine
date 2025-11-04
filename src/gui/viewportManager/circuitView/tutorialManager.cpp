#include "tutorialManager.h"

#include "circuitView.h"
#include "environment/environment.h"
#include "events/customEvents.h"

TutorialManager::TutorialManager(Environment* environment, CircuitView* circuitView) :
	circuitView(circuitView), elementCreator(circuitView->getViewportId()), environment(environment), tutorialRunning(false) { }

void TutorialManager::StartTutorial() {
	if (tutorialRunning) return;
	tutorialRunning = true;
	tutorialState = 0;
	circuit_id_t circuitId = circuitView->getBackend()->getCircuitManager().createNewCircuit(false);
	std::optional<evaluator_id_t> evaluatorId = circuitView->getBackend()->createEvaluator(circuitId);
	if (!evaluatorId) return;
	circuitView->setEvaluator(circuitView->getBackend(), evaluatorId.value());
	
	evaluator = circuitView->getBackend()->getEvaluator(evaluatorId.value());
	evaluator->setPause(false);
	SharedCircuit circuit = circuitView->getBackend()->getCircuitManager().getCircuit(circuitId);
	curentCircuit = circuitView->getBackend()->getCircuitManager().getCircuit(circuitId);
	curentCircuit->connectListener(this, std::bind(&TutorialManager::checkTutorial, this, std::placeholders::_1, std::placeholders::_2));
	circuitView->getEventRegister().registerFunction("CircuitStateSet", [this](const Event* event) -> bool {
		const StateSetEvent* stateSetEvent = event->cast<StateSetEvent>();
		if (!stateSetEvent) return false;
		this->checkTutorialState(stateSetEvent->getPosition(), stateSetEvent->getState());
		return false;
	});
	basicTutorial();
}

void TutorialManager::Stop() {
	if (!tutorialRunning) return;
	if (curentCircuit) {
		curentCircuit->disconnectListener(this);
	}
	elementCreator.clear();
	tutorialRunning = false;
}
void TutorialManager::checkTutorial(DifferenceSharedPtr, circuit_id_t) { basicTutorial(); }
void TutorialManager::checkTutorialState(Position pos, bool state) { 
	logInfo(pos);
	basicTutorial(); 
}

void TutorialManager::basicTutorial() {
	const BlockContainer* blockContainer = curentCircuit->getBlockContainer();
	// circuitView->getEventRegister().doEvent(DeltaXYEvent("view pan", 1, 1));
	// viewManager.setViewCenter(FPosition(2, 1));
	if (tutorialState == 0) {
		basicTutorialPart1();
	}
	if (tutorialState == 1 && blockContainer->getBlock(Position(0, 0)) != nullptr && blockContainer->getBlock(Position(0, 0))->type() == BlockType::SWITCH &&
		blockContainer->getBlock(Position(0, 2)) != nullptr && blockContainer->getBlock(Position(0, 2))->type() == BlockType::SWITCH) {
		tutorialState++;
	}
	if (tutorialState == 2) {
		basicTutorialPart2();
	}
	if (tutorialState == 3 && blockContainer->getBlock(Position(2, 1)) != nullptr && blockContainer->getBlock(Position(2, 1))->type() == BlockType::AND) {
		tutorialState++;
	}
	if (tutorialState == 4) {
		basicTutorialPart3();
	}
	if (tutorialState == 5 && blockContainer->connectionExists(Position(0, 0), Position(2, 1)) && blockContainer->connectionExists(Position(0, 2), Position(2, 1))) {
		tutorialState++;
	}
	if (tutorialState == 6) {
		basicTutorialPart4();
	}
	if (tutorialState == 7 && blockContainer->getBlock(Position(4, 1)) != nullptr && blockContainer->getBlock(Position(4, 1))->type() == BlockType::LIGHT && blockContainer->connectionExists(Position(2, 1), Position(4, 1))) {
		tutorialState++;
	}
	if (tutorialState == 8) {
		basicTutorialPart5();
	}
	if (tutorialState == 9 && evaluator->getState(Address(Position(2, 1))) == logic_state_t::HIGH) {
		tutorialState++;
	}
	if (tutorialState == 10) {
		basicTutorialPart6();
	}
	if (tutorialState == 11 && blockContainer->getBlock(Position(2, 0)) != nullptr && blockContainer->getBlock(Position(2, 0))->type() == BlockType::LIGHT) {
		Stop();
	}
}

void TutorialManager::basicTutorialPart1() {
	logInfo("Welcome to the Connetion Machine tutorial.");
	logInfo("Click the 'Switch' button on the left side menu, and click to place 2 switches where prompted.");
	elementCreator.addBlockPreview(BlockPreview(environment->getBlockRenderDataFeeder().getBlockRenderDataId(BlockType::SWITCH), Position(0, 0), Orientation()));
	elementCreator.addBlockPreview(BlockPreview(environment->getBlockRenderDataFeeder().getBlockRenderDataId(BlockType::SWITCH), Position(0, 2), Orientation()));
	tutorialState++;
}
void TutorialManager::basicTutorialPart2() {
	logInfo("Click the 'AND' button on the left side menu, and click to place an AND block where prompted.");
	logInfo("The AND block takes in any amount of inputs, and outputs 'ON' only when ALL inputs are 'ON' and outputs 'OFF' otherwise.");
	elementCreator.addBlockPreview(BlockPreview(environment->getBlockRenderDataFeeder().getBlockRenderDataId(BlockType::AND), Position(2, 1), Orientation()));
	tutorialState++;
}
void TutorialManager::basicTutorialPart3() {
	logInfo(
		"Click the 'Connection' button on the left side menu, and click the first switch followed by the AND block to connect the switch's output to the AND block's "
		"input."
	);
	logInfo("Repeat for the second switch.");
	elementCreator.addConnectionPreview(ConnectionPreview(FPosition(0, 0), FPosition(2, 1)));
	elementCreator.addConnectionPreview(ConnectionPreview(FPosition(0, 2), FPosition(2, 1)));
	tutorialState++;
}
void TutorialManager::basicTutorialPart4() {
	logInfo("Finally, click the 'LIGHT' button and place a light, then connect the AND block's output to the LIGHT.");
	elementCreator.addBlockPreview(BlockPreview(environment->getBlockRenderDataFeeder().getBlockRenderDataId(BlockType::LIGHT), Position(4, 1), Orientation()));
	elementCreator.addConnectionPreview(ConnectionPreview(FPosition(2, 1), FPosition(4, 1)));
	tutorialState++;
}
void TutorialManager::basicTutorialPart5() {
	logInfo("Switch to the 'state changer' tool in order to flip the switches. Play around with them so you understand what the AND block does.");
	tutorialState++;
}
void TutorialManager::basicTutorialPart6() {
	logInfo("Place a light where prompted to move on to the next tutorial.");
	elementCreator.addBlockPreview(BlockPreview(environment->getBlockRenderDataFeeder().getBlockRenderDataId(BlockType::LIGHT), Position(2, 0), Orientation()));
	tutorialState++;
}