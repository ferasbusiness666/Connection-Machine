#include "./circuitTestFileManager.h"
#include "backend/circuitTests/circuitTestGroup.h"
#include "backend/evaluator/simulator/logicState.h"
#include "logging/logging.h"
#include <utility>

std::optional<logic_state_t> stringToLogicState(const std::string& str) {
    if (str == "LOW" || str == "L") return (logic_state_t)0;
    if (str == "HIGH" || str == "H") return (logic_state_t)1;
    if (str == "FLOATING" || str == "Z") return (logic_state_t)2;
    if (str == "UNDEFINED" || str == "X") return (logic_state_t)3;
    return std::nullopt;
}

std::optional<CircuitTestGroup> CircuitTestFileManager::getCircuitTestFromFilePath(const std::string &path) {
    logInfo("Parsing test file (.tst)", "CircuitTestFileManager");
	std::ifstream inputFile(path, std::ios::in | std::ios::binary);
	if (!inputFile.is_open()) {
		logError("Couldn't open file at path: " + path, "CircuitTestFileManager");
		return std::nullopt;
	}

    std::string token;
    inputFile >> token;

    unsigned int version;
    const unsigned int latestVersion = 0;
    if (token == "version_0") {
        version = 0;
    } else {
        logError("Invalid test case file version: {}", "CircuitTestFileManager", token);
        return std::nullopt;
    }

    std::string testName;
    inputFile >> token;
    if (token == "Test:") {
        inputFile >> std::quoted(testName);
    } else {
        logError("Invalid file format, expected 'Test:' on line 2", "CircuitTestFileManager");
        return std::nullopt;
    }

    std::unordered_set<std::string> portNames = {};
    inputFile >> token;
    if (token == "Ports:") {
        inputFile >> std::ws;
        while(inputFile.peek() == '"') {
            std::string portName;
            inputFile >> std::quoted(portName);
            portNames.insert(portName);
            logInfo("New port name {}", "CircuitTestFileManager", portName);
            inputFile >> std::ws;
        }
    } else {
        logError("Invalid file format, expected 'Ports:' on line 3", "CircuitTestFileManager");
        return std::nullopt;
    }

    inputFile >> token;
    if (token == "Cases:") {
        inputFile >> std::ws;
    } else {
        logError("Invalid file format, expected 'Cases:' on line 4", "CircuitTestFileManager");
        return std::nullopt;
    }

    CircuitTestGroup testGroup(testName);

    inputFile >> std::ws;
    std::string groupName = "";
    while (inputFile.peek() == '"' && !inputFile.eof()) {
        inputFile >> std::ws;
        inputFile >> std::quoted(groupName);
        testGroup.addTestCase(groupName);
        logInfo("New group {}", "CircuitTestFileManager", groupName);
        inputFile >> std::ws;
        char curly;
        inputFile >> curly;
        if (curly != '{') {
            logError("Error parsing test file: unable to find curly brace after group name", "circuitTestFileManager");
            return std::nullopt;
        }
        inputFile >> std::ws;
        // adjust logic here to better handle grouping maybe
        while (inputFile.peek() != '}' && !inputFile.eof()) {
            inputFile >> token;
            if (token[0] == '>') {
                if (token.substr(1) == "Set:" || token.substr(1) == "Check:") {
                    std::vector<std::pair<std::string, logic_state_t>> states = {};
                    inputFile >> std::ws;
                    while (inputFile.peek() != '>' && inputFile.peek() != '}' && !inputFile.eof()) {
                        std::string portName;
                        inputFile >> std::quoted(portName);
                        if (!portNames.contains(portName)) {
                            logError("Error parsing test file: unrecognized port name '{}', be sure to include it at the top of the file", "CircuitTestFileManager", portName);
                            return std::nullopt;
                        }
                        inputFile >> std::ws;
                        char colon;
                        inputFile >> colon;
                        int portState;
                        inputFile >> portState;
                        if (portState < 0 || portState > 3) {
                            logWarning("Warning parsing test file: unrecognized state {} for port {}", "CircuitTestFileManager", portState, portName);
                        }
                        inputFile >> std::ws;
                        states.push_back(std::make_pair(portName, (logic_state_t)portState));
                    }
                    if (token.substr(1) == "Set:") {
                        testGroup.addSetStatesCommand(groupName, states);
                    } else {
                        testGroup.addCheckStatesCommand(groupName, states);
                    }
                } else if (token.substr(1) == "Step") {
                    int ticks;
                    inputFile >> ticks;
                    testGroup.addTickStepCommand(groupName, ticks);
                } else {
                    logError("Error parsing test file: unknown command '{}'", "CircuitTestFileManager", token.substr(1));
                    return std::nullopt;
                }
            } else {
                logError("Error parsing test file: looking for '>', did not find", "CircuitTestFileManager");
                return std::nullopt;
            }
        }
        inputFile >> curly;
        inputFile >> std::ws;
    }

    logInfo("Loaded test", "CircuitTestFileManager");
    return testGroup;
}

bool CircuitTestFileManager::saveToFile(const std::string& path, CircuitTestGroup& testGroup){
	std::ofstream outputFile(path);
	if (!outputFile.is_open()) {
		logError("Couldn't open file at path: {}", "CircuitTestFileManager", path);
		return false;
	}

    outputFile << "version_0\n";
    outputFile << "Test: \"" << testGroup.getName() << "\"\n";
    std::vector<std::string> ports;

    return true;
}
