#ifndef tutorialloader_h
#define tutorialloader_h

#include "gui/viewportManager/circuitView/tutorial.h"

std::pair<std::string,std::vector<TutorialStep>> parseTutorialFile(std::string fileName);

#endif