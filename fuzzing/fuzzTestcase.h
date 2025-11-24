#ifndef fuzzTestcase_h
#define fuzzTestcase_h

#include "backend/position/position.h"
#include "backend/container/block/blockDefs.h"
#include "backend/evaluator/simulator/logicState.h"

struct PlaceBlockAction {
	Position position;
	BlockType blockType;
	Orientation orientation;
};

struct RemoveBlockAction {
	Position position;
};

struct CreateConnectionAction {
	Position source;
	Position destination;
};

struct RemoveConnectionAction {
	Position source;
	Position destination;
};

struct SetBlockStateAction {
	Position position;
	logic_state_t state;
};

struct TickEvalAction {
	unsigned int numTicks;
};

using FuzzEditAction = std::variant<
	PlaceBlockAction,
	RemoveBlockAction,
	CreateConnectionAction,
	RemoveConnectionAction
>;

using FuzzTestAction = std::variant<
	SetBlockStateAction,
	TickEvalAction
>;

class FuzzTestcase {
public:
	FuzzTestcase() = default;

	void addEditAction(const FuzzEditAction& action) {
		editActions.push_back(action);
	}

	void addTestAction(const FuzzTestAction& action) {
		testActions.push_back(action);
	}

	const std::vector<FuzzEditAction>& getEditActions() const {
		return editActions;
	}

	const std::vector<FuzzTestAction>& getTestActions() const {
		return testActions;
	}

	void removeEditAction(size_t index) {
		if (index < editActions.size()) {
			editActions.erase(editActions.begin() + index);
		}
	}

	void removeTestAction(size_t index) {
		if (index < testActions.size()) {
			testActions.erase(testActions.begin() + index);
		}
	}

	bool getRunRealistic() const {
		return runRealistic;
	}

	void setRealistic(bool realistic) {
		runRealistic = realistic;
	}

private:
	std::vector<FuzzEditAction> editActions;
	std::vector<FuzzTestAction> testActions;
	bool runRealistic;
};

#endif /* fuzzTestcase_h */