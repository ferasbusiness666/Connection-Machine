#include "tutorialLoader.h"
#include "fileLoader.h"

void TutorialLoader::parseTutorialFile(std::string& fileName) {
	std::vector<char> fileData = readFileAsBytes("TutorialLib/" + fileName);
	std::string word;
	std::vector<std::string> words;
	std::vector<TutorialStep> steps;
	for (int i = 0; i < fileData.size(); i++) {
		if (fileData[i] == '\n' || fileData[i] == '\t' || fileData [i] == ' ') {
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

	for (; index < words.size(); index++) {
		if (words[index] == "Step:") {
			index++;
			if (words[index] == "Condition:") {
				if (words[index] == "Block:") {
					std::string blockName(words[++index]);
					words[++index].pop_back();
					Position pos(std::stoi(words[index].substr(words[index].find('(') + 1)), std::stoi(words[++index].substr(0, words[index].size() - 1)));
					std::string orietnation(words[++index]);
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

	return;
}