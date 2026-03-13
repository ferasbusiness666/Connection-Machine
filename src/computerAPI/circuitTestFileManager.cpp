#include "./circuitTestFileManager.h"
#include "backend/circuitTests/circuitTestGroup.h"
#include "backend/evaluator/simulator/logicState.h"
#include "logging/logging.h"
#include <optional>
#include <utility>

std::optional<logic_state_t> stringToLogicState(const std::string& str) {
    if (str == "LOW" || str == "L" || str == "0") return (logic_state_t)0;
    if (str == "HIGH" || str == "H" || str == "1") return (logic_state_t)1;
    if (str == "FLOATING" || str == "Z" || str == "2") return (logic_state_t)2;
    if (str == "UNDEFINED" || str == "X" || str == "3") return (logic_state_t)3;
    return std::nullopt;
}

std::optional<CircuitTestGroup> CircuitTestFileManager::getCircuitTestGroupFromFilePath(const std::string &path) {
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

    CircuitTestGroup testGroup(testName, false);

    std::set<std::string> inputs;
    inputFile >> token;
    if (token == "Inputs:") {
        inputFile >> std::ws;
        while(inputFile.peek() == '"') {
            std::string portName;
            inputFile >> std::quoted(portName);
            testGroup.addInput(portName);
            inputs.insert(portName);
            logInfo("New input name {}", "CircuitTestFileManager", portName);
            inputFile >> std::ws;
        }
    } else {
        logError("Invalid file format, expected 'Inputs:' on line 3", "CircuitTestFileManager");
        return std::nullopt;
    }

    std::set<std::string> outputs;
    inputFile >> token;
    if (token == "Outputs:") {
        inputFile >> std::ws;
        while(inputFile.peek() == '"') {
            std::string portName;
            inputFile >> std::quoted(portName);
            testGroup.addOutput(portName);
            outputs.insert(portName);
            logInfo("New output name {}", "CircuitTestFileManager", portName);
            inputFile >> std::ws;
        }
    } else {
        logError("Invalid file format, expected 'Outputs:' on line 4", "CircuitTestFileManager");
        return std::nullopt;
    }

    inputFile >> token;
    if (token == "Cases:") {
        inputFile >> std::ws;
    } else {
        logError("Invalid file format, expected 'Cases:' on line 5", "CircuitTestFileManager");
        return std::nullopt;
    }

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
                        if (token.substr(1) == "Set:" && !inputs.contains(portName)) {
                            logError("Error parsing test file: unrecognized input port name '{}', be sure to include it at the top of the file", "CircuitTestFileManager", portName);
                            return std::nullopt;
                        }
                        if (token.substr(1) == "Check:" && !outputs.contains(portName)) {
                            logError("Error parsing test file: unrecognized input port name '{}', be sure to include it at the top of the file", "CircuitTestFileManager", portName);
                            return std::nullopt;
                        }
                        inputFile >> std::ws;
                        char colon;
                        inputFile >> colon;
                        std::string portStateStr;
                        inputFile >> portStateStr;
                        std::optional<logic_state_t> portState = stringToLogicState(portStateStr);
                        if (portState == std::nullopt) {
                            logError("Error parsing test file: Unrecognized port state '{}'", "CircuitTestFileManager", portStateStr);
                            return std::nullopt;
                        }
                        inputFile >> std::ws;
                        states.push_back(std::make_pair(portName, portState.value()));
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

    outputFile << "Inputs: ";
    for (auto it = testGroup.getInputIterator(); it != testGroup.getInputIteratorEnd(); it++) {
        outputFile << "\"" << *it << "\" ";
    }

    outputFile << "\nOutputs: ";
    for (auto it = testGroup.getOutputIterator(); it != testGroup.getOutputIteratorEnd(); it++) {
        outputFile << "\"" << *it << "\"";
    }

    outputFile << "\nCases:\n";

    int i = 0;
    const CircuitTestGroup::TestCase* testCase = testGroup.getTestCase(i);
    while (testCase != nullptr) {
        outputFile << "\"" << testCase->name << "\" {\n";
        for (int j=0; j<testCase->testCommands.size(); j++) {
            const CircuitTestGroup::TestCommand testCommand = testCase->testCommands[j];
            if (testCommand.type == CircuitTestGroup::TestCommandType::NOP_COMMAND) {
                logInfo(">Command NOP", "CircuitTestFileManager");
            } else if (testCommand.type == CircuitTestGroup::TestCommandType::SET_STATES) {
                logInfo(">Command SET", "CircuitTestFileManager");
                outputFile << "\t>Set:\n";
                for (int x=0; x<testCommand.states.size(); x++) {
                    outputFile << "\t\t\"" << testCommand.states[x].first << "\":" << (unsigned int)testCommand.states[x].second << "\n";
                    logInfo(">>Port {}: {}", "CircuitTestFileManager", testCommand.states[x].first, (unsigned int)testCommand.states[x].second);
                }
            } else if (testCommand.type == CircuitTestGroup::TestCommandType::CHECK_STATES) {
                logInfo(">Command CHECK", "CircuitTestFileManager");
                outputFile << "\t>Check:\n";
                for (int x=0; x<testCommand.states.size(); x++) {
                    outputFile << "\t\t\"" << testCommand.states[x].first << "\":" << (unsigned int)testCommand.states[x].second << "\n";
                    logInfo(">>Port {}: {}", "CircuitTestFileManager", testCommand.states[x].first, (unsigned int)testCommand.states[x].second);
                }
            } else if (testCommand.type == CircuitTestGroup::TestCommandType::TICK_STEP) {
                logInfo(">Command STEP {}", "CircuitTestFileManager", testCommand.ticks);
                outputFile << "\t>Step " << testCommand.ticks << "\n";
            } else {
                logInfo(">Command UNKNOWN");
            }
        }
        outputFile << "}\n";
        i++;
        testCase = testGroup.getTestCase(i);
    }

    return true;
}
