#include "tutorialLoader.h"
#include "backend/container/block/blockDefs.h"
#include "backend/position/position.h"
#include "computerAPI/circuits/textParser.h"
#include "gui/viewportManager/circuitView/tutorial.h"
#include "logging/logging.h"
#include <cctype>
#include <string>
#include <unordered_map>

bool parsePosition(std::stringstream& ss, int line, Position& out) {
	char ch;
	int x;
	int y;
	if (!(ss >> ch) || ch != '(') {
		logError("Invalid position. Expected '(' on line {}.", "TutorialLoader", line);
		return false;
	}
	if (!(ss >> x)) {
		logError("Invalid position. Expected x coordinate on line {}.", "TutorialLoader", line);
		return false;
	}
	if (!(ss >> ch) || ch != ',') {
		logError("Invalid position. Expected ',' on line {}.", "TutorialLoader", line);
		return false;
	}
	if (!(ss >> y)) {
		logError("Invalid position. Expected y coordinate on line {}.", "TutorialLoader", line);
		return false;
	}
	if (!(ss >> ch) || ch != ')') {
		logError("Invalid position. Expected ')' on line {}.", "TutorialLoader", line);
		return false;
	}

	out = Position(x, y);
	return true;
}

void parsePreSteps(std::vector<std::string>& info, std::unordered_map<std::string, std::string>& macros, const std::vector<std::string>& lines) {
	macros.clear();
	for (int i = 0; i < lines.size(); i++) {
		std::stringstream ss(lines[i]);
		std::string tok;
		ss >> tok;
		if (tok == "version_0" || tok == "version_1") {
			// do something with versions once there is not backwards compatibility
		} else if ((tok == "Tutorial:")) {
			// Tutorial:
			if (!(ss >> info[0])) {
				logError("Invalid name on line {}, name should exist.", "TutorialLoader", i + 1);
				continue;
			}
		} else if (tok.starts_with("$")) {
			// Macro (i.e. '$p1 (2, 5)')
			if (tok.size() == 1) {
				logError("Invalid macro on line {}, macro must contain a variable name.", "TutorialLoader", i + 1);
				continue;
			}
			std::string value;
			value = ss.str().substr(ss.str().find(tok) + tok.length() + 1);
			if (!macros.emplace("(" + tok + ")", value).second) {
				logError("Warning: Duplicate macro definition on line {}.", "TutorialLoader", i + 1);
				continue;
			}
		}
	}
}

void macroReplace(std::vector<std::string>& lines, std::unordered_map<std::string, std::string>& macros) {
	for (std::string& line : lines) {
		for (auto& macro : macros) {
			size_t pos = 0;
			while ((pos = line.find(macro.first, pos)) != std::string::npos) {
				line.replace(pos, macro.first.length(), macro.second);
				pos += macro.second.length();
			}
		}
	}
}

void parseBlock(std::stringstream& ss, int line, BlockType& blockTypeOut, Position& posOut, Orientation& orientationOut) {
	// name (x,y) orientation (optional)
	std::string tok;
	std::string tmp;
	ss >> tok;
	blockTypeOut = stringToBlockType(tok);
	parsePosition(ss, line, posOut);
	ss >> tok;
	orientationOut = stringToOrientation((tok));
}

void parseConnection(std::stringstream& ss, int line, Position& p1out, Position& p2out) {
	// (x,y) (x,y)
	parsePosition(ss, line, p1out);
	parsePosition(ss, line, p2out);
}

void parseCondition(TutorialCondition& condition, const std::vector<std::string>& lines, int& line) {
	for (int i = line; i < lines.size(); i++) {
		std::stringstream ss(lines[i]);
		std::string tok;
		std::string tmp;
		ss >> tok;
		if (tok == "Step:" || tok == "Action:") {
			line = i - 1;
			return;
		} else if (tok == "Block:") {
			BlockType blockName;
			Position p;
			Orientation orientation;
			parseBlock(ss, i, blockName, p, orientation);
			condition.blocks.emplace_back(p, blockName, orientation);
		} else if (tok == "Connection:") {
			Position connectionOutput;
			Position connectionInput;
			parseConnection(ss, i, connectionOutput, connectionInput);
			condition.connections.emplace_back(connectionOutput, connectionInput);
		} else if (tok == "State:") {
			// (logic_state: l,h,z,x) (x,y) (numsteps)
			ss >> tok;
			std::optional<logic_state_t> state = stringToLogicState(tok);
			if (!state.has_value()) {
				logError("Incorrectly formatted state on line {}.", "TutorialLoader", i + 1);
			}
			Position pos;
			parsePosition(ss, i, pos);
			int numSteps;
			ss >> numSteps;
			condition.logicStates.emplace_back(pos, state.value(), numSteps);
		}
	}
}

void parseAction(TutorialAction& action, const std::vector<std::string>& lines, int& line) {
	for (int i = line; i < lines.size(); i++) {
		std::stringstream ss(lines[i]);
		std::string tok;
		std::string tmp;
		ss >> tok;
		if (tok == "Step:" || tok == "Condition:") {
			line = i - 1;
			return;
		} else if (tok == "Message:") {
			// name (x,y) orientation (optional)
			std::string message;
			message = ss.str().substr(ss.str().find(tok) + tok.length() + 1);
			if (message.starts_with("\"") && message.ends_with("\"")) {
				message = message.substr(1, message.length() - 2);
			}
			action.messages.emplace_back(message);
		} else if (tok == "Block") {
			// (name) (x,y) (orientation-optional)
			ss >> tok; // throw out 'Preview:'
			BlockType blockName;
			Position p;
			Orientation orientation;
			parseBlock(ss, i, blockName, p, orientation);
			action.blockPreviews.emplace_back(p, blockName, orientation);
		} else if (tok == "Connection") {
			// (x,y) (x,y)
			ss >> tok; // throw out 'Preview:'
			Position connectionOutput;
			Position connectionInput;
			parseConnection(ss, i, connectionOutput, connectionInput);
			action.connectionPreviews.emplace_back(connectionOutput, connectionInput);
		}
	}
}

void parseStep(std::vector<TutorialStep>& steps, const std::vector<std::string>& lines, int& line) {
	TutorialAction action;
	TutorialCondition condition;
	for (int i = line + 1; i < lines.size(); i++) {
		std::stringstream ss(lines[i]);
		std::string tok;
		ss >> tok;
		if (tok == "Step:" || i + 1 == lines.size()) {
			line = i - 1;
			steps.emplace_back(condition, action);
			return;
		} else if (tok == "Action:") {
			parseAction(action, lines, i);
		} else if (tok == "Condition:") {
			parseCondition(condition, lines, i);
		}
	}
}

void parseSteps(std::vector<TutorialStep>& steps, const std::vector<std::string>& lines) {
	for (int i = 0; i < lines.size(); i++) {
		std::stringstream ss(lines[i]);
		std::string tok;
		ss >> tok;
		if (tok == "Step:") {
			parseStep(steps, lines, i);
		}
	}
}

std::vector<TutorialStep> parseTutorialFile(std::string fileName) {
	logInfo("Loading tutorial '" + fileName + "'", "TutorialLoader");
	std::ifstream istream("TutorialLib/" + fileName);
	if (!istream.is_open()) {
		logError("Failed to open file '" + fileName + "'", "TutorialLoader");
		return {};
	}
	std::vector<std::string> lines;
	std::string buffer;
	while (std::getline(istream, buffer)) {
		lines.push_back(buffer);
	}
	std::vector<std::string> info(2, "");
	std::unordered_map<std::string, std::string> macros;
	parsePreSteps(info, macros, lines);
	macroReplace(lines, macros);
	std::vector<TutorialStep> steps;
	parseSteps(steps, lines);
	return steps;
}
