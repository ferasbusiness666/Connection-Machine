#include "tutorialDataManager.h"
#include "computerAPI/tutorialLoader.h"
#include "computerAPI/directoryManager.h"

void TutorialDataManager::initializeTutorials() {
	tutorialsMutex.lock();
	tutorialNamesMutex.lock();
	std::vector<std::string> filenames;
	logInfo("Tutorials Loaded:", "MainWindow");
	logInfo((DirectoryManager::getResourceDirectory() / "tutorials" / "").string());
	for (const auto& file : std::filesystem::directory_iterator((DirectoryManager::getResourceDirectory() / "tutorials" / "").string())) {
		filenames.push_back(file.path().filename().string());
		logInfo("  [" + std::to_string(filenames.size()) + "] " + file.path().filename().string());
	}
	std::pair<std::string, std::vector<TutorialStep>> curTutorial;
	for (const std::string& tutorial : filenames) {
		curTutorial = loadTutorialFromFile(tutorial);
		tutorialNames.emplace_back(curTutorial.first);
		tutorials.emplace(curTutorial);
	}
	tutorialsMutex.unlock();
	tutorialNamesMutex.unlock();
}

std::pair<std::string, std::vector<TutorialStep> > loadTutorialFromFile(std::string filename) { return parseTutorialFile(filename); }
