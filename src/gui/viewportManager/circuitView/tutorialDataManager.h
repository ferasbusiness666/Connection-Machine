#ifndef tutorialDataManager_h
#define tutorialDataManager_h

#include <vector>
#include "gui/viewportManager/circuitView/tutorial.h"

std::vector<TutorialStep> loadTutorialFromFile(std::string filename);

#endif