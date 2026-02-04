#include "tutorial.h"

#include "backend/position/position.h"
#include "circuitView.h"
#include "computerAPI/tutorialLoader.h"

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
	std::optional<simulator_id_t> simulatorId = circuitView->getBackend().createSimulator(circuitId);
	if (!simulatorId) return;
	circuitView->setSimulatoruator(simulatorId.value());

	simulator = circuitView->getBackend().getSimulator(simulatorId.value());
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
	const TutorialStep& currentStep = tutorialSteps[tutorialState];
	const BlockContainer& blockContainer = curentCircuit->getBlockContainer();

	for (const TutorialCondition::BlockRequirement& blockCondition : currentStep.condition.blocks) {
		const Block* currentBlock = blockContainer.getBlock(blockCondition.pos);
		if (currentBlock == nullptr) {
			return false;
		}
		if (currentBlock->type() != blockCondition.type) {
			return false;
		}
		if (currentBlock->getOrientation() != blockCondition.orientation) {
			return false;
		}
	}
	for (const TutorialCondition::ConnectionRequirement& connectionCondition : currentStep.condition.connections) {
		if (!blockContainer.connectionExists(connectionCondition.pos1, connectionCondition.pos2)) {
			return false;
		}
	}
	for (const TutorialCondition::LogicStateRequirement stateCondition : currentStep.condition.logicStates) {
		// simulator->tickStep(stateCondition.numSteps);
		logic_state_t b = simulator->getState(Address(stateCondition.pos));
		if (simulator->getState(Address(stateCondition.pos)) != stateCondition.state) {
			logic_state_t a = simulator->getState(Address(stateCondition.pos));
			return false;
		}
	}
	return true;
}

void Tutorial::runCurrentStep() {
	if (tutorialState >= tutorialSteps.size()) return;
	const TutorialStep& currentStep = tutorialSteps[tutorialState];
	const BlockContainer& blockContainer = curentCircuit->getBlockContainer();
	// change this later to real popups or something
	for (const std::string& message : currentStep.action.messages) {
		logInfo(message);
	}
	for (const TutorialAction::BlockPreviewInfo& blockPreview : currentStep.action.blockPreviews) {
		elementCreator.addBlockPreview(
			BlockPreview(environment.getBlockRenderDataFeeder().getBlockRenderDataId(blockPreview.type), blockPreview.pos, blockPreview.orientation)
		);
	}
	for (const TutorialAction::ConnectionPreviewInfo& connectionPreview : currentStep.action.connectionPreviews) {
		std::optional<FVector> optionalPos1Offset;
		const Block* block1 = blockContainer.getBlock(connectionPreview.pos1);
		if (block1 != nullptr) {
			optionalPos1Offset = blockContainer.getBlockDataManager()
									 .getBlockData(block1->type())
									 ->getConnectionPortOffset(blockContainer.getOutputConnectionEnd(connectionPreview.pos1).value().getConnectionId());
		} else {
			for (const TutorialAction::BlockPreviewInfo& blockPreview : currentStep.action.blockPreviews) {
				if (connectionPreview.pos1.withinArea(
						blockPreview.pos,
						blockPreview.pos +
							(blockPreview.orientation * blockContainer.getBlockDataManager().getBlockData(blockPreview.type)->getSize()).getLargestVectorInArea()
					)) {
					optionalPos1Offset = blockContainer.getBlockDataManager().getBlockData(blockPreview.type)->getConnectionPortOffset((connection_end_id_t)0);
				}
			}
		}
		std::optional<FVector> optionalPos2Offset;
		const Block* block2 = blockContainer.getBlock(connectionPreview.pos2);
		if (block2 != nullptr) {
			optionalPos2Offset = blockContainer.getBlockDataManager()
									 .getBlockData(block2->type())
									 ->getConnectionPortOffset(blockContainer.getInputConnectionEnd(connectionPreview.pos2).value().getConnectionId());
		} else {
			for (const TutorialAction::BlockPreviewInfo& blockPreview : currentStep.action.blockPreviews) {
				if (connectionPreview.pos2.withinArea(
						blockPreview.pos,
						blockPreview.pos +
							(blockPreview.orientation * blockContainer.getBlockDataManager().getBlockData(blockPreview.type)->getSize()).getLargestVectorInArea()
					)) {
					optionalPos2Offset = blockContainer.getBlockDataManager().getBlockData(blockPreview.type)->getConnectionPortOffset((connection_end_id_t)0);
				}
			}
		}
		if (!optionalPos1Offset.has_value()) {
			optionalPos1Offset = FVector(0.5, 0.5);
		}
		if (!optionalPos2Offset.has_value()) {
			optionalPos2Offset = FVector(0.5, 0.5);
		}
		elementCreator.addConnectionPreview(ConnectionPreview(
			FPosition(connectionPreview.pos1.x, connectionPreview.pos1.y) + optionalPos1Offset.value(),
			FPosition(connectionPreview.pos2.x, connectionPreview.pos2.y) + optionalPos2Offset.value()
		));
	}
}

void Tutorial::forceCompleteStep() {
	if (!tutorialRunning) return;
	if (tutorialState >= tutorialSteps.size()) return;
	const TutorialStep& currentStep = tutorialSteps[tutorialState];
	const BlockContainer& blockContainer = curentCircuit->getBlockContainer();

	for (const TutorialCondition::BlockRequirement& block : currentStep.condition.blocks) {
		const Block* currentBlock = blockContainer.getBlock(block.pos);
		if (currentBlock != nullptr) {
			curentCircuit->tryRemoveBlock(block.pos);
		}
		curentCircuit->tryInsertBlock(block.pos, block.orientation, block.type);
	}

	for (const TutorialCondition::ConnectionRequirement& connection : currentStep.condition.connections) {
		const Block* currentBlock1 = blockContainer.getBlock(connection.pos1);
		const Block* currentBlock2 = blockContainer.getBlock(connection.pos2);
		if (currentBlock1 == nullptr) {
			curentCircuit->tryInsertBlock(connection.pos1, Rotation::ZERO, BlockType::JUNCTION);
		}
		if (currentBlock2 == nullptr) {
			curentCircuit->tryInsertBlock(connection.pos2, Rotation::ZERO, BlockType::JUNCTION);
		}
		curentCircuit->tryCreateConnection(connection.pos1, connection.pos2);
	}

	for (const TutorialCondition::LogicStateRequirement stateCondition : currentStep.condition.logicStates) {
		const Block* currentBlock = blockContainer.getBlock(stateCondition.pos);
		if (currentBlock != nullptr) {
			simulator->setState(Address(stateCondition.pos), stateCondition.state);
			// simulator->tickStep(1);
		}
	}

	advanceTutorial();
	simulator->tickStep(1);
}