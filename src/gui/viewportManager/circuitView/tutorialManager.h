#ifndef tutorialManager_h
#define tutorialManager_h

class CircuitView;
class Environment;

#include "renderer/elementCreator.h"

class TutorialManager {
public:
	TutorialManager(Environment* environment, CircuitView* circuitView);
	void StartTutorial();
	void Stop();

private:
	void checkTutorial(DifferenceSharedPtr, circuit_id_t);

	SharedCircuit curentCircuit;

	CircuitView* circuitView;
	Environment* environment;
	ElementCreator elementCreator;
};

#endif