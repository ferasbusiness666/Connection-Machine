#ifndef tutorialManager_h
#define tutorialManager_h

#include "backend/container/difference.h"
#include "./viewManager/viewManager.h"
#include "renderer/elementCreator.h"

class CircuitView;
class Environment;

struct TutorialCondition {
	struct BlockRequirement {
		Position pos;
		BlockType type;
		Orientation orientation;
	};
	struct ConnectionRequirement {
		Position pos1;
		Position pos2;
	};
	struct LogicStateRequirement {
		Position pos;
		logic_state_t state;
		int numSteps;
	};
	struct TruthTable {
		std::vector<Position> force;
		std::vector<Position> expect;
		std::vector<logic_state_t> states;
		int numSteps;
	};
	std::vector<BlockRequirement> blocks;
	std::vector<ConnectionRequirement> connections;
	std::vector<LogicStateRequirement> logicStates;
	TruthTable truthTable;
};

struct TutorialAction {
	struct BlockPreviewInfo {
		Position pos;
		BlockType type;
		Orientation orientation;
	};
	struct ConnectionPreviewInfo {
		Position pos1;
		Position pos2;
	};
	std::vector<std::string> messages;
	std::vector<BlockPreviewInfo> blockPreviews;
	std::vector<ConnectionPreviewInfo> connectionPreviews;
	std::optional<FPosition> viewCenter;
};

struct TutorialStep {
	TutorialCondition condition;
	TutorialAction action;
};

class Tutorial {
public:

	Tutorial(Environment& environment, CircuitView& circuitView);
	void StartTutorial();
	void stop();
	void setTutorial(const std::vector<TutorialStep>& steps);
	void forceCompleteStep();

	std::string selectTutorial();

private:
	void checkTutorial(DifferenceSharedPtr diff, circuit_id_t circuitId);
	void checkTutorialState(Position pos, logic_state_t state);

	void runCurrentStep();
	bool isCurrentStepComplete() const;
	void advanceTutorial();

	CircuitView& circuitView;
	ElementCreator elementCreator;
	Environment& environment;
	EvalLogicSimulator* simulator;
	SharedCircuit currentCircuit = nullptr;
	ViewManager& viewManager;
	DataUpdateEventManager::DataUpdateEventReceiver dataUpdateEventReciever;

	bool tutorialRunning;
	int tutorialState;
	std::vector<TutorialStep> tutorialSteps;
};

#endif /* tutorialManager_h */
