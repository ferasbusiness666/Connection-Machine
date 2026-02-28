#include "./testCaseFileManager.h"
#include "logging/logging.h"

std::optional<CircuitTestCase> TestCaseFileManager::getCircuitTestCaseFromFilePath(const std::string& path) {
	logInfo("Parsing test file (.tst)", "TestCaseFileManager");
	std::ifstream inputFile(path, std::ios::in | std::ios::binary);
	if (!inputFile.is_open()) {
		logError("Couldn't open file at path: " + path, "TestCaseFileManager");
		return std::nullopt;
	}

	std::string token;
	inputFile >> token;

	unsigned int version;
	const unsigned int latestVersion = 0;
	if (token == "version_0") {
		version = 0;
	} else {
		logError("Invalid test case file version: {}", "TestCaseFileManager", token);
		return std::nullopt;
	}

	while (inputFile >> token) { }

	return std::nullopt;
}