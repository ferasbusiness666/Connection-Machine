#include "tutorialLoader.h"
#include "computerAPI/circuits/textParser.h"
#include "fileLoader.h"

Position getPositionFromString(const std::string str1, const std::string str2) {
	return Position(std::stoi(str1.substr(str1.find('(') + 1)), std::stoi(str2.substr(0, str2.size() - 1)));
}

void parseTutorialFile(std::string& fileName) {
	std::ifstream istream("TutorialLib/" + fileName);
	std::vector<TutorialStep> steps;
	std::string cur;
	int line = 0;
	istream >> cur;
	line++;
	if (cur != "version_0") {
		throw std::string("tutorial/parser version mismatch\nError on line: " + std::to_string(line) + "\n");
	}
	istream >> cur;
	line++;
	if (cur != "Tutorial:") {
		throw std::string("tutorial missing name\nError on line: " + std::to_string(line) + "\n");
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
						throw std::string("incorrectly formatted condition\nError on line: " + std::to_string(line) + "\n");
					}
				}

				if (cur == "Action:") {
					TutorialAction a;
					while (true) {
						if (!(istream >> cur)) {
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
							throw std::string("incorrectly formatted action\nError on line: " + std::to_string(line) + "\n");
						}
					}
				} else {
					throw std::string("incorrect format (missing 'Action:' or 'Condition:')\nError on line: " + std::to_string(line) + "\n");
				}
			} else {
				throw std::string("incorrect format (missing 'Step:')\nError on line: " + std::to_string(line) + "\n");
			}
		}
		steps.push_back(step);
		if (!(istream >> cur)) {
			break;
		}
	}

	return;
}
