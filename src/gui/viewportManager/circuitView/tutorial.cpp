#include "tutorial.h"
#include "circuitView.h"

#include "environment/environment.h"
#include "events/customEvents.h"

Tutorial::Tutorial(Environment& environment, CircuitView& circuitView) :
	circuitView(&circuitView), elementCreator(circuitView.getViewportId()), environment(environment), tutorialRunning(false), tutorialState(0) { }

void Tutorial::StartTutorial() {
	if (tutorialRunning) return;
	if (tutorialSteps.empty()) return;
	tutorialRunning = true;
	tutorialState = 0;
	circuit_id_t circuitId = circuitView->getBackend().getCircuitManager().createNewCircuit(false);
	std::optional<evaluator_id_t> evaluatorId = circuitView->getBackend().createEvaluator(circuitId);
	if (!evaluatorId) return;
	circuitView->setEvaluator(evaluatorId.value());

	evaluator = circuitView->getBackend().getEvaluator(evaluatorId.value());
	SharedCircuit circuit = circuitView->getBackend().getCircuitManager().getCircuit(circuitId);
	curentCircuit = circuitView->getBackend().getCircuitManager().getCircuit(circuitId);
	curentCircuit->connectListener(this, std::bind(&Tutorial::checkTutorial, this, std::placeholders::_1, std::placeholders::_2));
	circuitView->getEventRegister().registerFunction("CircuitStateSet", [this](const Event* event) -> bool {
		const StateSetEvent* stateSetEvent = event->cast<StateSetEvent>();
		if (!stateSetEvent) return false;
		this->checkTutorialState(stateSetEvent->getPosition(), stateSetEvent->getState());
		return false;
	});
	runCurrentStep();
}

void Tutorial::Stop() {
	if (!tutorialRunning) return;
	if (curentCircuit) {
		curentCircuit->disconnectListener(this);
	}
	elementCreator.clear();
	tutorialRunning = false;
}
void Tutorial::checkTutorial(DifferenceSharedPtr, circuit_id_t) { advanceTutorial(); }
void Tutorial::checkTutorialState(Position pos, logic_state_t state) { advanceTutorial(); }

void Tutorial::setTutorial(const std::vector<TutorialStep>& steps) { tutorialSteps = steps; }

void Tutorial::advanceTutorial() {
	if (!tutorialRunning) return;
	if (tutorialState >= tutorialSteps.size()) return;
	if (isCurrentStepComplete()) {
		tutorialState++;
		elementCreator.clear();
		if (tutorialState == tutorialSteps.size()) {
			Stop();
			return;
		}
		runCurrentStep();
	}
}

bool Tutorial::isCurrentStepComplete() const {
	if (tutorialState >= tutorialSteps.size()) return false;
	TutorialStep currentStep = tutorialSteps[tutorialState];
	BlockContainer blockContainer = curentCircuit->getBlockContainer();

	for (std::vector<TutorialCondition::BlockRequirement>::iterator it = currentStep.condition.blocks.begin(); it != currentStep.condition.blocks.end(); it++) {
		const Block* currentBlock = blockContainer.getBlock(it->pos);
		if (currentBlock == nullptr) {
			return false;
		}
		if (currentBlock->type() != it->type) {
			return false;
		}
		if (currentBlock->getOrientation() != it->orientation) {
			return false;
		}
	}
	for (std::vector<TutorialCondition::ConnectionRequirement>::iterator it = currentStep.condition.connections.begin(); it != currentStep.condition.connections.end();
		 it++) {
		if (!blockContainer.connectionExists(it->pos1, it->pos2)) {
			return false;
		}
	}
	for (std::vector<TutorialCondition::LogicStateRequirement>::iterator it = currentStep.condition.logicStates.begin(); it != currentStep.condition.logicStates.end();
		 it++) {
		evaluator->tickStep(it->numSteps);
		if (evaluator->getState(Address(it->pos)) != it->state) {
			return false;
		}
	}
	return true;
}

void Tutorial::runCurrentStep() {
	if (tutorialState >= tutorialSteps.size()) return;
	TutorialStep currentStep = tutorialSteps[tutorialState];
	BlockContainer blockContainer = curentCircuit->getBlockContainer();
	// change this later to real popups or something
	for (std::vector<std::string>::iterator it = currentStep.action.messages.begin(); it != currentStep.action.messages.end(); it++) {
		logInfo(*it);
	}
	for (std::vector<TutorialAction::BlockPreviewInfo>::iterator it = currentStep.action.blockPreviews.begin(); it != currentStep.action.blockPreviews.end(); it++) {
		elementCreator.addBlockPreview(BlockPreview(environment.getBlockRenderDataFeeder().getBlockRenderDataId(it->type), it->pos, it->orientation));
	}
	for (std::vector<TutorialAction::ConnectionPreviewInfo>::iterator it = currentStep.action.connectionPreviews.begin();
		 it != currentStep.action.connectionPreviews.end();
		 it++) {
		std::optional<FVector> optionalPos1Offset;
		if (blockContainer.getBlock(it->pos1) != nullptr) {
			optionalPos1Offset = blockContainer.getBlockDataManager()
									 .getBlockData(blockContainer.getBlock(it->pos1)->type())
									 ->getConnectionPortOffset(blockContainer.getOutputConnectionEnd(it->pos1).value().getConnectionId());
		} else {
			for (std::vector<TutorialAction::BlockPreviewInfo>::iterator it2 = currentStep.action.blockPreviews.begin(); it2 != currentStep.action.blockPreviews.end();
				 it2++) {
				if (it2->pos == it->pos1) {
					optionalPos1Offset = blockContainer.getBlockDataManager().getBlockData(it2->type)->getConnectionPortOffset((connection_end_id_t)0);
				}
			}
		}
		std::optional<FVector> optionalPos2Offset;
		if (blockContainer.getBlock(it->pos2) != nullptr) {
			optionalPos2Offset = blockContainer.getBlockDataManager()
									 .getBlockData(blockContainer.getBlock(it->pos2)->type())
									 ->getConnectionPortOffset(blockContainer.getInputConnectionEnd(it->pos2).value().getConnectionId());
		} else {
			for (std::vector<TutorialAction::BlockPreviewInfo>::iterator it2 = currentStep.action.blockPreviews.begin(); it2 != currentStep.action.blockPreviews.end();
				 it2++) {
				if (it2->pos == it->pos2) {
					optionalPos2Offset = blockContainer.getBlockDataManager().getBlockData(it2->type)->getConnectionPortOffset((connection_end_id_t)0);
				}
			}
		}
		if (!optionalPos1Offset.has_value()) {
			optionalPos1Offset = FVector(0.5, 0.5);
		}
		if (!optionalPos2Offset.has_value()) {
			optionalPos1Offset = FVector(0.5, 0.5);
		}
		elementCreator.addConnectionPreview(
			ConnectionPreview(FPosition(it->pos1.x, it->pos1.y) + optionalPos1Offset.value(), FPosition(it->pos2.x, it->pos2.y) + optionalPos2Offset.value())
		);
	}
}
