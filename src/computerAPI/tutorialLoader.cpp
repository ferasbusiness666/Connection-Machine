#include "tutorialLoader.h"
#include "computerAPI/circuits/textParser.h"
#include "gui/viewportManager/circuitView/tutorial.h"
#include "logging/logging.h"
#include <cctype>
#include <string>
#include <unordered_map>
#include <regex>

Position getPositionFromString(const std::string str1, const std::string str2) {
	return Position(std::stoi(str1.substr(str1.find('(') + 1)), std::stoi(str2.substr(0, str2.size() - 1)));
}

void parsePreSteps(std::vector<std::string>& info, std::unordered_map<std::string, std::string>& macros, const std::vector<std::string>& lines) {
	info.clear();
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
			if (!(ss >> value)) {
				logError("Invalid macro on line {}, macro must contain a value.", "TutorialLoader", i);
				continue;
			}
			value = ss.str().substr(ss.str().find(value));
			logInfo(value);
			if (!macros.emplace(tok, value).second) {
				logError("Warning: Duplicate macro definition on line {}.", "TutorialLoader", i);
				continue;
			}
		} else if (tok.starts_with("#")) {
			// ignore comments
			continue;
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

void parseSteps(std::vector<TutorialStep>& steps, const std::vector<std::string>& lines) {
	for (int i = 0; i < lines.size(); i++) {
        
    }
}

std::vector<TutorialStep> parseTutorialFile(std::string fileName) {
	logInfo("Loading tutorial '" + fileName + "'", "TutorialLoader");
	std::ifstream istream("TutorialLib/" + fileName);
	if (!istream.is_open()) {
		logInfo("Failed to open file '" + fileName + "'", "TutorialLoader");
	}
	std::vector<std::string> lines;
	std::string buffer;
	while (std::getline(istream, buffer)) {
		lines.push_back(buffer);
	}
	std::vector<std::string> info(2, "");
	std::unordered_map<std::string, std::string> macros;
	parsePreSteps(info, macros, lines);
	std::vector<TutorialStep> steps;
	std::string cur;
	int line = 0;
	istream >> cur;
	line++;
	if (cur != "version_0") {
		logError("tutorial/parser version mismatch\nError on line: {}", "parseTutorialFile", line);
		return {};
	}
	istream >> cur;
	line++;
	if (cur != "Tutorial:") {
		logError("tutorial missing name\nError on line: {}", "parseTutorialFile", line);
		return {};
	}
	std::string name;

	istream >> std::quoted(name);
	istream >> cur;
	line++;
	for (;;) {
		TutorialStep step;
		if (cur == "Step:") {
			istream >> cur;
			line++;
			if (cur == "Condition:") {
				TutorialCondition c;
				while (true) {
					istream >> cur;
					line++;
					if (cur == "Block:") {
						istream >> cur;
						BlockType blockName(stringToBlockType(cur));
						std::string tmp;
						istream >> tmp;
						istream >> cur;
						tmp.pop_back();
						Position p = getPositionFromString(tmp, cur);
						istream >> cur;
						Orientation orietnation(stringToOrientation((cur)));
						c.blocks.emplace_back(p, blockName, orietnation);
					} else if (cur == "Connection:") {
						std::string px;
						std::string py;
						istream >> px;
						istream >> py;
						px.pop_back();
						Position p1 = getPositionFromString(px, py);
						istream >> px;
						istream >> py;
						px.pop_back();
						Position p2 = getPositionFromString(px, py);
						c.connections.emplace_back(p1, p2);
					} else if (cur == "State:") {
						std::string stateStr;
						istream >> stateStr;
						logic_state_t state;
						if (stateStr == "h") {
							state = logic_state_t::HIGH;
						} else if (stateStr == "l") {
							state = logic_state_t::LOW;
						}
						std::string px;
						std::string py;
						istream >> px;
						istream >> py;
						px.pop_back();
						Position pos = getPositionFromString(px, py);
						int numSteps;
						istream >> numSteps;
						c.logicStates.emplace_back(pos, state, numSteps);
					} else if (cur == "Action:") {
						// do nothing and let it go to Action check
						step.condition = c;
						c.blocks.clear();
						c.connections.clear();
						c.logicStates.clear();
						break;
					} else {
						logError("incorrectly formatted condition\nError on line: {}", "parseTutorialFile", line);
						return {};
					}
				}

				if (cur == "Action:") {
					TutorialAction a;
					while (true) {
						if (!(istream >> cur)) {
							step.action = a;
							break;
						}
						line++;
						if (cur == "Message:") {
							std::string msg;
							istream >> std::quoted(msg);
							a.messages.push_back(msg);
						} else if (cur == "Block") {
							istream >> cur;
							istream >> cur;
							BlockType blockName(stringToBlockType(cur));
							std::string tmp;
							istream >> tmp;
							istream >> cur;
							tmp.pop_back();
							Position p = getPositionFromString(tmp, cur);
							istream >> cur;
							Orientation orientation(stringToOrientation(cur));
							a.blockPreviews.emplace_back(p, blockName, orientation);
						} else if (cur == "Connection") {
							istream >> cur;
							std::string px;
							std::string py;
							istream >> px;
							istream >> py;
							px.pop_back();
							Position p1 = getPositionFromString(px, py);
							istream >> px;
							istream >> py;
							px.pop_back();
							Position p2 = getPositionFromString(px, py);
							a.connectionPreviews.emplace_back(p1, p2);
						} else if (cur == "Step:") {
							// do nothing and let it go back to Step:
							step.action = a;
							break;
						} else {
							logError("incorrectly formatted action\nError on line: {}", "parseTutorialFile", line);
							return {};
						}
					}
				} else {
					logError("incorrect format (missing 'Action:' or 'Condition:'\nError on line: {}", "parseTutorialFile", line);
					return {};
				}
			} else {
				logError("incorrect format (missing 'Step:'\nError on line: {}", "parseTutorialFile", line);
				return {};
			}
		}
		steps.push_back(step);
		if (istream.peek() == EOF) {
			break;
		}
	}

	return steps;
}
