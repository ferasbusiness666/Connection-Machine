#ifndef fuzzTestcase_h
#define fuzzTestcase_h

#include "backend/position/position.h"
#include "backend/container/block/blockDefs.h"
#include "backend/evaluator/simulator/logicState.h"

#include <nlohmann/json.hpp>

struct PlaceBlockAction {
	Position position;
	BlockType blockType;
	Orientation orientation;
	nlohmann::json toJson() const {
		nlohmann::json j;
		j["type"] = "PlaceBlockAction";
		j["position"] = { {"x", position.x}, {"y", position.y} };
		j["blockType"] = static_cast<uint16_t>(blockType);
		j["orientation"] = { {"rotation", static_cast<uint8_t>(orientation.rotation)}, {"flipped", orientation.flipped} };
		return j;
	}
};

struct RemoveBlockAction {
	Position position;
	nlohmann::json toJson() const {
		nlohmann::json j;
		j["type"] = "RemoveBlockAction";
		j["position"] = { {"x", position.x}, {"y", position.y} };
		return j;
	}
};

struct CreateConnectionAction {
	Position source;
	Position destination;
	nlohmann::json toJson() const {
		nlohmann::json j;
		j["type"] = "CreateConnectionAction";
		j["source"] = { {"x", source.x}, {"y", source.y} };
		j["destination"] = { {"x", destination.x}, {"y", destination.y} };
		return j;
	}
};

struct RemoveConnectionAction {
	Position source;
	Position destination;
	nlohmann::json toJson() const {
		nlohmann::json j;
		j["type"] = "RemoveConnectionAction";
		j["source"] = { {"x", source.x}, {"y", source.y} };
		j["destination"] = { {"x", destination.x}, {"y", destination.y} };
		return j;
	}
};

struct SetBlockStateAction {
	Position position;
	logic_state_t state;
	nlohmann::json toJson() const {
		nlohmann::json j;
		j["type"] = "SetBlockStateAction";
		j["position"] = { {"x", position.x}, {"y", position.y} };
		j["state"] = static_cast<uint8_t>(state);
		return j;
	}
};

struct TickEvalAction {
	unsigned int numTicks;
	nlohmann::json toJson() const {
		nlohmann::json j;
		j["type"] = "TickEvalAction";
		j["numTicks"] = numTicks;
		return j;
	}
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

	std::string serialize() const;
	static FuzzTestcase deserialize(const std::string& data);

private:
	std::vector<FuzzEditAction> editActions;
	std::vector<FuzzTestAction> testActions;
	bool runRealistic;
};

#endif /* fuzzTestcase_h */