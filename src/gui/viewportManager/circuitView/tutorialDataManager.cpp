#include "tutorialDataManager.h"
#include "computerAPI/tutorialLoader.h"
#include "computerAPI/directoryManager.h"

void TutorialDataManager::initializeTutorials() {
	tutorialsMutex.lock();
	tutorialNamesMutex.lock();
	logInfo("Tutorials Loaded:", "MainWindow");
	logInfo((DirectoryManager::getResourceDirectory() / "tutorials" / "").string());
	for (const auto& file : std::filesystem::directory_iterator((DirectoryManager::getResourceDirectory() / "tutorials" / "").string())) {
		auto pair = tutorials.emplace(loadTutorialFromFile(file.path().string()));
		tutorialNames.emplace_back(pair.first->first);
	}
	tutorialsMutex.unlock();
	tutorialNamesMutex.unlock();
}

std::pair<std::string, std::vector<TutorialStep> > loadTutorialFromFile(std::string filePath) { return parseTutorialFile(filePath); }
