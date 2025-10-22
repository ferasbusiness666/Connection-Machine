#ifndef tutorialManager_h
#define tutorialManager_h

class CircuitView;

#include "renderer/elementCreator.h"

class TutorialManager {
    public:
        TutorialManager(CircuitView* circuitView);
        void StartTutorial();
    private:

    CircuitView* circuitView;
    ElementCreator elementCreator;

};

#endif