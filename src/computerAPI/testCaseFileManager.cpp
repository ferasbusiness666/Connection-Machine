#include "./testCaseFileManager.h"
#include "backend/evaluator/simulator/logicState.h"
#include "logging/logging.h"

std::optional<logic_state_t> stringToLogicState(const std::string& str) {
    if (str == "LOW" || str == "L") return (logic_state_t)0;
    if (str == "HIGH" || str == "H") return (logic_state_t)1;
    if (str == "FLOATING" || str == "Z") return (logic_state_t)2;
    if (str == "UNDEFINED" || str == "X") return (logic_state_t)3;
    return std::nullopt;
}

std::optional<CircuitTestCase> TestCaseFileManager::getCircuitTestCaseFromFilePath(const std::string &path) {
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
    }
    else {
        logError("Invalid test case file version: {}", "TestCaseFileManager", token);
        return std::nullopt;
    }

    std::string testName;
    inputFile >> token;

    if (token == "Test:") {
        inputFile >> std::quoted(testName);
    } else {
        logError("Invalid file format, expected 'Test:' on line 2", "TestCaseFileManager");
        return std::nullopt;
    }

    std::vector<std::string> portNames = {};
    inputFile >> token;
    if (token == "Ports:") {
        while(inputFile.peek() == '"') {
            std::string portName;
            inputFile >> std::quoted(portName);
            portNames.push_back(portName);
        }
    } else {
        logError("Invalid file format, expected 'Ports:' on line 3", "TestCaseFileManager");
        return std::nullopt;
    }



    while (inputFile >> token) {
        
    }

    logInfo("Loaded test", "TestCaseFileManager");
    return std::nullopt;
}