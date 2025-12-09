#ifndef tutorialDataManager_h
#define tutorialDataManager_h

#include <vector>
// #include "circuitView.h"
#include "gui/viewportManager/circuitView/tutorial.h"


std::vector<TutorialStep> loadTutorialFromFile(std::string filename);
// this class is used for loading tutorials of all types: "loadTutorialFromXXXX"

#endif