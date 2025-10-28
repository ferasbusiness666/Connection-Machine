#include "tutorialManager.h"

#include "circuitView.h"

TutorialManager::TutorialManager(CircuitView* circuitView) : circuitView(circuitView), 
                                        elementCreator(circuitView->getViewportId()) {

        
    }
        
