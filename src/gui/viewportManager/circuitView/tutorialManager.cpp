#include "tutorialManager.h"

#include "circuitView.h"
#include "environment/environment.h"

TutorialManager::TutorialManager(Environment* environment, CircuitView* circuitView) : circuitView(circuitView), elementCreator(circuitView->getViewportId()), environment(environment) {}

void TutorialManager::StartTutorial() {
	circuit_id_t circuitId = circuitView->getBackend()->getCircuitManager().createNewCircuit(false);
	std::optional<evaluator_id_t> evaluatorId = circuitView->getBackend()->createEvaluator(circuitId);
	if (!evaluatorId) return;
	circuitView->setEvaluator(circuitView->getBackend(), evaluatorId.value());
	elementCreator.addBlockPreview(BlockPreview(
		environment->getBlockRenderDataFeeder().getBlockRenderDataId(BlockType::AND),
		Position(),
		Orientation()
	));
	elementCreator.addSelectionElement(SelectionElement(
		Position(1, 2), true
	));
	curentCircuit = circuitView->getBackend()->getCircuitManager().getCircuit(circuitId);

	curentCircuit->connectListener(this, std::bind(&TutorialManager::checkTutorial, this, std::placeholders::_1, std::placeholders::_2));
}

void TutorialManager::Stop() {
	if (curentCircuit) {
		curentCircuit->disconnectListener(this);
	}
	elementCreator.clear();
}
void TutorialManager::checkTutorial(DifferenceSharedPtr, circuit_id_t) {
	logInfo("Changed happened");
}
