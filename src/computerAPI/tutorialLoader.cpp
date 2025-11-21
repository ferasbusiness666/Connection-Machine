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
	istream >> cur;
	if (cur != "version_0") {
		throw std::string("tutorial/parser version mismatch\n");
	}
	istream >> cur;
	if (cur != "Tutorial:") {
		throw std::string("tutorial missing name\n");
	}
	std::string name;

	istream >> std::quoted(name);

	for (; istream >> cur;) {
		TutorialStep step;
		if (cur == "Step:") {
			istream >> cur;
			if (cur == "Condition:") {
				TutorialCondition c;
				while (true) {
					istream >> cur;
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
						std::string p1a;
						std::string p1b;
						std::string p2a;
						std::string p2b;
						istream >> p1a;
						istream >> p1b;
						istream >> p2a;
						istream >> p2b;
						p1a.pop_back();
						p2a.pop_back();
						Position p1 = getPositionFromString(p1a, p1b);
						Position p2 = getPositionFromString(p2a, p2b);
						c.connections.emplace_back(p1, p2);
					} else if (cur == "State") {
						std::string stateStr;
						istream >> stateStr;
						logic_state_t state;
						if (stateStr == "h") {
							state = logic_state_t::HIGH;
						} else if (stateStr == "l") {
							state = logic_state_t::LOW;
						}
						std::string p1;
						std::string p2;
						istream >> p1;
						istream >> p2;
						p1.pop_back();
						Position pos = getPositionFromString(p1, p2);
						int numSteps;
						istream >> numSteps;
						c.logicStates.emplace_back(pos, state, numSteps);
					} else if (cur == "Action:") {
						// do nothing and let it go to Action check
						step.condition = c;
						break;
					} else {
						throw std::string("incorrectly formatted condition\n");
					}
				}

				if (cur == "Action:") {
					TutorialAction a;
					while (true) {
						if (!(istream >> cur)) {
							break;
						}
						if (cur == "Message:") {
							std::string msg;
							istream >> std::quoted(msg);
							a.messages.push_back(msg);
						} else if (cur == "Block") {
							istream >> cur;
							istream >> cur;
							BlockType blockName(stringToBlockType(cur));
							std::string tmp1;
							std::string tmp2;
							istream >> tmp1;
							istream >> tmp2;
							tmp1.pop_back();
							Position p = getPositionFromString(tmp1, tmp2);
							istream >> cur;
							Orientation orientation(stringToOrientation(cur));
							a.blockPreviews.emplace_back(p, blockName, orientation);
						} else if (cur == "Connection") {
							istream >> cur;
							std::string p1a;
							std::string p1b;
							std::string p2a;
							std::string p2b;
							istream >> p1a;
							istream >> p1b;
							istream >> p2a;
							istream >> p2b;
							p1a.pop_back();
							p2a.pop_back();
							Position p1 = getPositionFromString(p1a, p1b);
							Position p2 = getPositionFromString(p2a, p2b);
							a.connectionPreviews.emplace_back(p1, p2);
						} else if (cur == "Step:") {
							// do nothing and let it go back to Step:
							step.action = a;
							break;
						} else {
							throw std::string("incorrectly formatted action\n" + cur);
						}
					}
				} else {
					throw std::string("incorrect format (missing 'Action:' or 'Condition:')\n");
				}
			} else {
				throw std::string("incorrect format (missing 'Step:')\n");
			}
		}
		steps.push_back(step);
	}

	return;
}
