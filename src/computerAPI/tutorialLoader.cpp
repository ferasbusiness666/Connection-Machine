#include "tutorialLoader.h"
#include "backend/container/block/blockDefs.h"
#include "backend/position/position.h"
#include "computerAPI/circuits/textParser.h"
#include "gui/viewportManager/circuitView/tutorial.h"
#include "logging/logging.h"
#include <cctype>
#include <regex>
#include <string>
#include <unordered_map>

Position getPositionFromString(std::string& str1, std::string& str2, int line) {
	int num1;
	int num2;
	if (isdigit(str1[1])) {
		num1 = std::stoi(str1.substr(1));
	} else {
		logError("Invalid position on line {}.", "TutorialLoader", line);
		num1 = 0;
	}
	if (str1.ends_with(')')) {
		int index = str1.find(',') + 1;
		if (isdigit(str1[index])) {
			num2 = std::stoi(str1.substr(index));
		} else {
			logError("Invalid position on line {}.", "TutorialLoader", line);
			num2 = 0;
		}
	} else {
		if (isdigit(str2[0])) {
			num2 = std::stoi(str2);
		} else {
			logError("Invalid position on line {}.", "TutorialLoader", line);
			num2 = 0;
		}
	}
	return Position(num1, num2);
}

void parsePreSteps(std::vector<std::string>& info, std::unordered_map<std::string, std::string>& macros, const std::vector<std::string>& lines) {
	macros.clear();
	for (int i = 0; i < lines.size(); i++) {
		std::stringstream ss(lines[i]);
		std::string tok;
		ss >> tok;
		if (tok == "version_0") {
			// use version_0
		} else if ((tok == "Tutorial:")) {
			// Tutorial:
			if (!(ss >> info[0])) {
				logError("Invalid name on line {}, name must exist.", "TutorialLoader", i);
				continue;
			}
		} else if (tok.starts_with("$")) {
			// Macro (i.e. '$p1 (2, 5)')
			if (tok.size() == 1) {
				logError("Invalid macro on line {}, macro must contain a variable name.", "TutorialLoader", i);
				continue;
			}
			std::string value;
			value = ss.str().substr(tok.length() + 1);
			logInfo(value);
			if (!macros.emplace("(" + tok + ")", value).second) {
				logError("Warning: Duplicate macro definition on line {}.", "TutorialLoader", i);
				continue;
			}
		}
	}
}

void macroReplace(std::vector<std::string>& lines, std::unordered_map<std::string, std::string>& macros) {
	for (int i = 0; i < lines.size(); i++) {
		for (auto& macro : macros) {
			lines[i] = std::regex_replace(lines[i], (std::regex)macro.first, macro.second);
		}
	}
}

void parseBlock(std::stringstream& ss, int line, BlockType& blockTypeOut, Position& posOut, Orientation& orientationOut) {
	// name (x,y) orientation (optional)
	std::string tok;
	std::string tmp;
	ss >> tok;
	blockTypeOut = stringToBlockType(tok);
	ss >> tmp;
	ss >> tok;
	posOut = getPositionFromString(tmp, tok, line);
	ss >> tok;
	orientationOut = stringToOrientation((tok));
}

void parseConnection(std::stringstream& ss, int line, Position& p1out, Position& p2out) {
	// (x,y) (x,y)
	std::string tok;
	std::string tmp;
	ss >> tok;
	ss >> tmp;
	p1out = getPositionFromString(tok, tmp, line);
	ss >> tok;
	ss >> tmp;
	p2out = getPositionFromString(tok, tmp, line);
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
				logError("Incorrectly formatted state on line {}.", "TutorialLoader", i);
			}
			ss >> tok;
			ss >> tmp;
			Position pos = getPositionFromString(tok, tmp, i);
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
			message = ss.str().substr(tok.length() + 1);
			action.messages.emplace_back(message);
		} else if (tok == "Block") {
			// (name) (x,y) (orientation-optional)
			BlockType blockName;
			Position p;
			Orientation orientation;
			parseBlock(ss, i, blockName, p, orientation);
			action.blockPreviews.emplace_back(p, blockName, orientation);
		} else if (tok == "Connection") {
			// (x,y) (x,y)
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
		if (tok == "Step:") {
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
		logInfo("Failed to open file '" + fileName + "'", "TutorialLoader");
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
