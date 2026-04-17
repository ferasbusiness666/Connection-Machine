#ifndef tutorialDataManager_h
#define tutorialDataManager_h

#include "gui/viewportManager/circuitView/tutorial.h"

class TutorialDataManager {
public:
	TutorialDataManager() { initializeTutorials(); }
	const std::vector<TutorialStep> getTutorial(const std::string& tutorialName) { return tutorials[tutorialName]; }
	const std::vector<std::string> getNames() { return tutorialNames; }
	void initializeTutorials();
private:
	std::mutex tutorialNamesMutex;
	std::vector<std::string> tutorialNames;
	std::mutex tutorialsMutex;
	std::unordered_map<std::string, std::vector<TutorialStep>> tutorials;
};

void writeTutorial(const std::vector<TutorialStep>& steps, const std::string& tutorialName, const std::string& filepath);

#endif /* tutorialDataManager_h */
