#include "computerAPI/tutorialLoader.h"
#include "tutorialDataManager.h"
#include <vector>

std::vector<TutorialStep> loadTutorialFromFile(std::string filename) { return parseTutorialFile(filename); }
