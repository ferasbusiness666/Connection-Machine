#ifndef tutorialManager_h
#define tutorialManager_h

class CircuitView;
class Environment;

#include "renderer/elementCreator.h"
#include "./viewManager/viewManager.h"

class TutorialManager {
public:
	TutorialManager(Environment& environment, CircuitView& circuitView);
	void StartTutorial();
	void Stop();


private:
	void checkTutorial(DifferenceSharedPtr diff, circuit_id_t circuitId);
	void checkTutorialState(Position pos, bool state);

	void basicTutorial();
	void basicTutorialPart1();
	void basicTutorialPart2();
	void basicTutorialPart3();
	void basicTutorialPart4();
	void basicTutorialPart5();
	void basicTutorialPart6();

	SharedEvaluator evaluator;
	circuit_id_t curentCircuitId;

	CircuitView* circuitView;
	ViewManager viewManager;
	Environment& environment;
	ElementCreator elementCreator;

	int tutorialState;
	bool tutorialRunning;
};

#endif