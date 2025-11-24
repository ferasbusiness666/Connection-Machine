#include "fuzzTestcase.h"

std::string FuzzTestcase::serialize() const {
	nlohmann::json j;
	j["type"] = "FuzzTestcase Eval v1";
	j["runRealistic"] = runRealistic;
	j["editActions"] = nlohmann::json::array();
	j["testActions"] = nlohmann::json::array();
	for (const auto& action : editActions) {
		std::visit([&j](auto&& arg) {
			j["editActions"].push_back(arg.toJson());
		}, action);
	}
	for (const auto& action : testActions) {
		std::visit([&j](auto&& arg) {
			j["testActions"].push_back(arg.toJson());
		}, action);
	}
	return j.dump();

}

FuzzTestcase FuzzTestcase::deserialize(const std::string& data) {
	FuzzTestcase testcase;
	nlohmann::json j = nlohmann::json::parse(data);
	testcase.runRealistic = j.value("runRealistic", false);
	for (const auto& actionJson : j["editActions"]) {
		std::string type = actionJson.value("type", "");
		if (type == "PlaceBlockAction") {
			PlaceBlockAction action;
			action.position = Position(actionJson["position"]["x"], actionJson["position"]["y"]);
			action.blockType = static_cast<BlockType>(actionJson["blockType"].get<uint16_t>());
			action.orientation = Orientation(
				Rotation(actionJson["orientation"]["rotation"].get<uint8_t>()),
				actionJson["orientation"]["flipped"].get<bool>()
			);
			testcase.addEditAction(action);
		} else if (type == "RemoveBlockAction") {
			RemoveBlockAction action;
			action.position = Position(actionJson["position"]["x"], actionJson["position"]["y"]);
			testcase.addEditAction(action);
		} else if (type == "CreateConnectionAction") {
			CreateConnectionAction action;
			action.source = Position(actionJson["source"]["x"], actionJson["source"]["y"]);
			action.destination = Position(actionJson["destination"]["x"], actionJson["destination"]["y"]);
			testcase.addEditAction(action);
		} else if (type == "RemoveConnectionAction") {
			RemoveConnectionAction action;
			action.source = Position(actionJson["source"]["x"], actionJson["source"]["y"]);
			action.destination = Position(actionJson["destination"]["x"], actionJson["destination"]["y"]);
			testcase.addEditAction(action);
		}
	}
	for (const auto& actionJson : j["testActions"]) {
		std::string type = actionJson.value("type", "");
		if (type == "SetBlockStateAction") {
			SetBlockStateAction action;
			action.position = Position(actionJson["position"]["x"], actionJson["position"]["y"]);
			action.state = static_cast<logic_state_t>(actionJson["state"].get<uint8_t>());
			testcase.addTestAction(action);
		} else if (type == "TickEvalAction") {
			TickEvalAction action;
			action.numTicks = actionJson["numTicks"].get<unsigned int>();
			testcase.addTestAction(action);
		}
	}
	return testcase;
}