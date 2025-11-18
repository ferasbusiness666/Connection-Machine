#include "tutorialLoader.h"
#include "fileLoader.h"
#include "computerAPI/circuits/textParser.h"

Position getPositionFromString(const std::string str1, const std::string str2) {
	return Position(std::stoi(str1.substr(str1.find('(') + 1)), std::stoi(str2.substr(0, str2.size() - 1)));
}

void parseTutorialFile(std::string& fileName) {
	std::vector<char> fileData = readFileAsBytes("TutorialLib/" + fileName);
	std::string word;
	std::vector<std::string> words;
	std::vector<TutorialStep> steps;
	for (int i = 0; i < fileData.size(); i++) {
		if (fileData[i] == '\n' || fileData[i] == '\t' || fileData[i] == ' ') {
			if (word.empty()) {
				continue;
			} else {
				words.push_back(word);
				word.clear();
			}
		} else {
			word.push_back(fileData[i]);
		}
	}
	if (words[0] != "version_0") {
		throw std::string("tutorial/parser version mismatch\n");
	}
	if (words[1] != "Tutorial:") {
		throw std::string("tutorial missing name\n");
	}
	int index = 2;
	while (words[index++].back() != '"') {
		// get past name, prolly should do something but idk
	}
	TutorialCondition c;
	TutorialAction a;

	for (; index < words.size(); index++) {
		if (words[index] == "Step:") {
			index++;
			if (words[index] == "Condition:") {
				if (words[index] == "Block:") {
					BlockType blockName(stringToBlockType(words[++index]));
					words[++index].pop_back();
					Position p = getPositionFromString(words[index], words[index + 1]);
					index++;
					Orientation orietnation(stringToOrientation((words[++index])));
					c.blocks.emplace_back(p, blockName, orietnation);
				} else if (words[index] == "Connection:") {

				} else if (words[index] == "State") {

				} else if (words[index] == "Action:") {
					// do nothing and let it go to Action check
				} else {
					throw std::string("incorrectly formatted condition\n");
				}
				if (words[index] == "Action:") {
					index++;
					if (index == words.size()) {
						break;
					}
					if (words[index] == "Message:") {

					} else if (words[index] == "Block") {

					} else if (words[index] == "Connection") {

					} else if (words[index] == "Step:") {
						// do nothing and let it go back to Step:
					} else {
						throw std::string("incorrectly formatted action\n");
					}
				} else {
					throw std::string("incorrect format (missing 'Action:' or 'Condition:')\n");
				}
			} else {
				throw std::string("incorrect format (missing 'Step:')\n");
			}
		}
	}

	return;
}

