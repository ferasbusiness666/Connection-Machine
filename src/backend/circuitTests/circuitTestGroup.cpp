#include "circuitTestGroup.h"

#include "backend/evaluator/simulator/evalLogicSimulator.h"
#include "backend/evaluator/simulatorManager.h"
#include "backend/backend.h"

CircuitTestGroup::CircuitTestGroupCopy CircuitTestGroup::getMinimalCopy() {
    return CircuitTestGroupCopy(name, isTruthTable, truthTableTicks, testCases, inputs, outputs);
}

void CircuitTestGroup::sendTestGroupUpdate() {
    backend.getDataUpdateEventManager().sendEvent("testGroupUpdate", name);
}

bool CircuitTestGroup::addTestCase(std::string name, int id) {
    if (testCaseNameToID.contains(name)) {
        logError("Cannot add test case '{}' due to one with the same name already existing", "CircuitTestGroup", name);
        return false;
    }
    if (id <= -1) {
        testCases.emplace_back(name);
        testCaseNameToID.emplace(std::make_pair(name, testCases.size()-1));
        sendTestGroupUpdate();
        return true;
    }
    if (id >= testCases.size()) {
        logError("Attempted insertion at position {} in test case vector exceeds its bounds ({})", "CircuitTestGroup", id, testCases.size());
        return false;
    }
    else {
        testCases.insert(testCases.begin() + id, TestCase(name));
        for (int i=0; i<testCases.size(); i++) {
            std::string name = testCases[i].name;
            testCaseNameToID.emplace(std::make_pair(name, i));
        }
    }
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::removeTestCase(std::string name) {
    if (!testCaseNameToID.contains(name)) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", name);
        return false;
    }
    int id = testCaseNameToID[name];
    return removeTestCase(id);
}

bool CircuitTestGroup::removeTestCase(int id) {
    if (testCases.size() <= id || id < 0) {
        logError("ID {} is out of the bounds of the test cases vector ({})", "CircuitTestGroup", id, testCases.size());
        return false;
    }
    testCases.erase(testCases.begin() + id);
    testCaseNameToID.clear();
    for (int i=0; i<testCases.size(); i++) {
        std::string name = testCases[i].name;
        testCaseNameToID.emplace(std::make_pair(name, i));
    }
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::swapTestCases(std::string testCase1, std::string testCase2){
    if (!testCaseNameToID.contains(testCase1)) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", testCase1);
        return false;
    }
    if (!testCaseNameToID.contains(testCase2)) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", testCase2);
        return false;
    }
    if (testCase1 == testCase2) return false;
    sendTestGroupUpdate();
    return swapTestCases(testCaseNameToID[testCase1], testCaseNameToID[testCase2]);
}

bool CircuitTestGroup::swapTestCases(int id1, int id2) {
    if (id1 >= testCases.size() || id1 < 0) {
        logError("Attempted swap at position {} in test case vector exceeds its bounds ({})", "CircuitTestGroup", id1, testCases.size());
        return false;
    }
    if (id2 >= testCases.size() || id2 < 0) {
        logError("Attempted swap at position {} in test case vector exceeds its bounds ({})", "CircuitTestGroup", id2, testCases.size());
        return false;
    }
    if (id1 == id2) return false;
    std::swap(testCases[id1], testCases[id2]);
    testCaseNameToID[testCases[id1].name] = id1;
    testCaseNameToID[testCases[id2].name] = id2;
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::renameTestCase(std::string oldName, std::string newName) {
    if (oldName == newName) return false;
    if (!testCaseNameToID.contains(oldName)) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", oldName);
        return false;
    }
    sendTestGroupUpdate();
    return renameTestCase(testCaseNameToID[oldName], newName);
}

bool CircuitTestGroup::renameTestCase(int id, std::string newName) {
    if (testCases.size() <= id || id < 0) {
        logError("ID {} is out of the bounds of the test cases vector ({})", "CircuitTestGroup", id, testCases.size());
        return false;
    }
    if (testCaseNameToID.contains(newName)) {
        logError("Cannot rename test case to '{}' due to one of the same name already existing", "CircuitTestGroup", newName);
        return false;
    }
    std::string oldName = testCases[id].name;
    testCases[id].name = newName;
    testCaseNameToID.erase(testCaseNameToID.find(oldName));
    testCaseNameToID.emplace(std::make_pair(newName, id));
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::addSimpleTestCase(std::string name, std::vector<std::pair<std::string, logic_state_t>> inputStates, std::vector<std::pair<std::string, logic_state_t>> outputStates){
    if (!isTruthTable) {
        logWarning("Action not recommended on non-truth table, performing anyways", "CircuitTestGroup");
    }
    if (testCaseNameToID.contains(name)) {
        logError("Cannot add simple test case '{}' due to one with the same name already existing", "CircuitTestGroup", name);
        return false;
    }
    testCaseNameToID.emplace(std::make_pair(name, testCases.size()-1));
    std::vector<TestCommand> testCommands;
    testCommands.emplace_back(SET_STATES, 0, inputStates);
    testCommands.emplace_back(TICK_STEP, truthTableTicks);
    testCommands.emplace_back(CHECK_STATES, 0, outputStates);
    testCases.emplace_back(name, testCommands);
    sendTestGroupUpdate();
    return true;
}

const CircuitTestGroup::TestCase* CircuitTestGroup::getTestCase(int id) {
    if (id >= testCases.size() || id < 0) return nullptr;
    return &testCases[id];
}

const CircuitTestGroup::TestCase* CircuitTestGroup::getTestCase(std::string name) {
    if (!testCaseNameToID.contains(name)) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", name);
        return nullptr;
    }
    return &testCases[testCaseNameToID[name]];
}

bool CircuitTestGroup::addSetStatesCommand(std::string testCaseName, std::vector<std::pair<std::string, logic_state_t>> states, int id) {
    if (isTruthTable) {
        logError("Method addSetStatesCommand disallowed on truth table, use addSimpleTestCase", "CircuitTestGroup");
        return false;
    }
    auto idIter = testCaseNameToID.find(testCaseName);
    if (idIter == testCaseNameToID.end()) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", testCaseName);
        return false;
    }
    TestCase* testCase = &testCases[idIter->second];
    if (id <= -1) {
        testCase->testCommands.emplace_back(SET_STATES, 0, states);
        sendTestGroupUpdate();
        return true;
    }
    if (id >= testCase->testCommands.size()) {
        logError("Attempted insertion at position {} in test command vector of test case '{}' exceeds its bounds ({})", "CircuitTestGroup", id, testCaseName, testCase->testCommands.size());
        return false;
    }
    else {
        testCase->testCommands.insert(testCase->testCommands.begin() + id, TestCommand(SET_STATES, 0, states));
    }
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::addCheckStatesCommand(std::string testCaseName, std::vector<std::pair<std::string, logic_state_t>> states, int id) {
    if (isTruthTable) {
        logError("Method addCheckStatesCommand disallowed on truth table, use addSimpleTestCase", "CircuitTestGroup");
        return false;
    }
    auto idIter = testCaseNameToID.find(testCaseName);
    if (idIter == testCaseNameToID.end()) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", testCaseName);
        return false;
    }
    TestCase* testCase = &testCases[idIter->second];
    if (id <= -1) {
        testCase->testCommands.emplace_back(CHECK_STATES, 0, states);
        sendTestGroupUpdate();
        return true;
    }
    if (id >= testCase->testCommands.size()) {
        logError("Attempted insertion at position {} in test command vector of test case '{}' exceeds its bounds ({})", "CircuitTestGroup", id, testCaseName, testCase->testCommands.size());
        return false;
    }
    else {
        testCase->testCommands.insert(testCase->testCommands.begin() + id, TestCommand(CHECK_STATES, 0, states));
    }
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::modifyStatesCommand(std::string testCaseName, std::vector<std::pair<std::string, logic_state_t>> states, int id) {
    if (isTruthTable) {
        logError("Method modifyStatesCommand disallowed on truth table, use addSimpleTestCase", "CircuitTestGroup");
        return false;
    }
    auto idIter = testCaseNameToID.find(testCaseName);
    if (idIter == testCaseNameToID.end()) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", testCaseName);
        return false;
    }
    TestCase* testCase = &testCases[idIter->second];
    if (id >= testCase->testCommands.size() || id < 0) {
        logError("Attempted modification at position {} in test command vector of test case '{}' exceeds its bounds ({})", "CircuitTestGroup", id, testCaseName, testCase->testCommands.size());
        return false;
    }
    if (testCase->testCommands[id].type != CHECK_STATES || testCase->testCommands[id].type != SET_STATES) {
        logError("Test command at position {} in test command vector of test case '{}' is not a state-related command", "CircuitTestGroup", id, testCaseName);
        return false;
    }
    testCase->testCommands[id].states = states;
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::addTickStepCommand(std::string testCaseName, int ticks, int id) {
    if (isTruthTable) {
        logError("Method addTickStepCommand disallowed on truth table, use addSimpleTestCase", "CircuitTestGroup");
        return false;
    }
    auto idIter = testCaseNameToID.find(testCaseName);
    if (idIter == testCaseNameToID.end()) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", testCaseName);
        return false;
    }
    TestCase* testCase = &testCases[idIter->second];
    if (id <= -1) {
        testCase->testCommands.emplace_back(TICK_STEP, ticks);
        sendTestGroupUpdate();
        return true;
    }
    if (id >= testCase->testCommands.size()) {
        logError("Attempted insertion at position {} in test command vector of test case '{}' exceeds its bounds ({})", "CircuitTestGroup", id, testCaseName, testCase->testCommands.size());
        return false;
    }
    else {
        testCase->testCommands.insert(testCase->testCommands.begin() + id, TestCommand(TICK_STEP, 0, {}));
    }
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::modifyTickStepCommand(std::string testCaseName, int ticks, int id) {
    if (isTruthTable) {
        logError("Method modifyStatesCommand disallowed on truth table, use addSimpleTestCase", "CircuitTestGroup");
        return false;
    }
    auto idIter = testCaseNameToID.find(testCaseName);
    if (idIter == testCaseNameToID.end()) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", testCaseName);
        return false;
    }
    TestCase* testCase = &testCases[idIter->second];
    if (id >= testCase->testCommands.size() || id < 0) {
        logError("Attempted modification at position {} in test command vector of test case '{}' exceeds its bounds ({})", "CircuitTestGroup", id, testCaseName, testCase->testCommands.size());
        return false;
    }
    if (testCase->testCommands[id].type != TICK_STEP) {
        logError("Test command at position {} in test command vector of test case '{}' is not a state-related command", "CircuitTestGroup", id, testCaseName);
        return false;
    }
    testCase->testCommands[id].ticks = ticks;
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::removeTestCommand(std::string testCaseName, int id) {
    if (isTruthTable) {
        logError("Method removeTestCommand disallowed on truth table", "CircuitTestGroup");
        return false;
    }
    if (!testCaseNameToID.contains(testCaseName)) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", testCaseName);
        return false;
    }
    TestCase* testCase = &testCases[testCaseNameToID[testCaseName]];
    if (id >= testCase->testCommands.size()) return false;
    testCase->testCommands.erase(testCase->testCommands.begin() + id);
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::swapTestCommands(std::string testCaseName, int id1, int id2){
    if (isTruthTable) {
        logError("Method swapTestCommands disallowed on truth table", "CircuitTestGroup");
        return false;
    }
    if (!testCaseNameToID.contains(testCaseName)) {
        logError("Unable to find test case with name '{}'", "CircuitTestGroup", testCaseName);
        return false;
    }
    TestCase* testCase = &testCases[testCaseNameToID[testCaseName]];
    if (id1 >= testCase->testCommands.size() || id1 < 0) {
        logError("Attempted swap at position {} in test command vector of test case '{}' exceeds its bounds ({})", "CircuitTestGroup", id1, testCaseName, testCase->testCommands.size());
        return false;
    }
    if (id2 >= testCase->testCommands.size() || id2 < 0) {
        logError("Attempted swap at position {} in test command vector of test case '{}' exceeds its bounds ({})", "CircuitTestGroup", id2, testCaseName, testCase->testCommands.size());
        return false;
    }
    std::swap(testCase->testCommands[id1], testCase->testCommands[id2]);
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::addInput(std::string input) {
    for (auto it = inputs.begin(); it != inputs.end(); it++) {
        if (*it == input) {
            logError("Input '{}' already exists in this test group", "CircuitTestGroup", input);
            return false;
        }
    }
    inputs.push_back(input);
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::addOutput(std::string output) {
    for (auto it = outputs.begin(); it != outputs.end(); it++) {
        if (*it == output) {
            logError("Output '{}' already exists in this test group", "CircuitTestGroup", output);
            return false;
        }
    }
    outputs.push_back(output);
    sendTestGroupUpdate();
    return true;
}

bool CircuitTestGroup::generateTestCircuit(BlockType blockType, simulator_id_t simulatorToUse) {
    CircuitManager& cirManager = backend.getCircuitManager();
    SimulatorManager& evalManager = backend.getSimulatorManager();
    BlockDataManager& blockDataManager = backend.getBlockDataManager();

	circuit_id_t circuitId;
	simulator_id_t simulatorId;
    if (simulatorToUse == 0) {
        circuitId = cirManager.createNewCircuit(false);
		simulatorId = backend.createSimulator(circuitId).value();
    } else {
		simulator = backend.getSimulator(simulatorToUse);
		if (simulator) {
			circuitId = simulator->getCircuitId();

		} else {
			logWarning("simulatorToUse was {}. But a simulator with that Id could not be found.", "CircuitTestGroup::generateTestCircuit", simulatorToUse);
			circuitId = cirManager.createNewCircuit(false);
			simulatorId = backend.createSimulator(circuitId).value();
		}
	}
    simulator = evalManager.getSimulator(simulatorId);
    simulator->setPause(true);
    SharedCircuit circuit = cirManager.getCircuit(circuitId);
    circuit->clear();
    const BlockContainer& blockContainer = circuit->getBlockContainer();

    const BlockData* blockData = blockDataManager.getBlockData(blockType);
    std::unordered_map<connection_end_id_t, BlockData::ConnectionData> connections = blockData->getConnections();
    // change implementation of this when BlockData::getConnectionNameToId is implemented
    namePositionMap.clear();

    if (!circuit->tryInsertBlock(Position(0,0), Orientation(), blockType)) {
        logError("Couldn't insert test circuit block '{}'", "circuitTestCase", blockType);
        return false;
    }

	const Block* testedBlock = circuit->getBlockContainer().getBlock(Position(0, 0));

    // iterate through the connections on the block type and create a switch/light to represent each input/output on a test circuit
    for (auto iter = connections.begin(); iter != connections.end(); iter++) {
        if (iter->second.portType == BlockData::ConnectionData::PortType::INPUT || iter->second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
            Position externalConnPos = Position(-1-namePositionMap.size(), 0);

            std::optional<std::string> blockNameOpt = blockData->getConnectionIdToName(iter->first);
            if (blockNameOpt == std::nullopt) {
                logError("Unable to resolve block name for id {}", "CircuitTestGroup", iter->first);
                return false;
            }
            std::string blockName = blockNameOpt.value();
            if (std::find(inputs.begin(), inputs.end(), blockName) == inputs.end()) {
                logWarning("Tested circuit's port '{}' does not match any inputs expected by test, this may cause errors", "CircuitTestGroup", blockName);
            }
            if (!circuit->tryInsertBlock(externalConnPos, Orientation(), SWITCH)) {
                logError("Couldn't insert switch test circuit block", "CircuitTestGroup");
                return false;
            }

            const Block* block = blockContainer.getBlock(externalConnPos);
            if (!circuit->tryCreateConnection(ConnectionEnd(testedBlock->id(), iter->first), ConnectionEnd(block->id(), 0))) {
                logError("Couldn't create switch test circuit connection, ext: {}", "CircuitTestGroup", externalConnPos);
                return false;
            }
            namePositionMap.insert({blockData->getConnectionIdToName(iter->first).value(), externalConnPos});
        }
        if (iter->second.portType == BlockData::ConnectionData::PortType::OUTPUT || iter->second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
            Position externalConnPos = Position(-1-namePositionMap.size(), 0);

            std::optional<std::string> blockNameOpt = blockData->getConnectionIdToName(iter->first);
            if (blockNameOpt == std::nullopt) {
                logError("Unable to resolve block name for id {}", "CircuitTestGroup", iter->first);
                return false;
            }
            std::string blockName = blockNameOpt.value();
            if (std::find(outputs.begin(), outputs.end(), blockName) == outputs.end()) {
                logWarning("Tested circuit's port '{}' does not match any outputs expected by test, this may cause errors", "CircuitTestGroup", blockName);
            }
            if (!circuit->tryInsertBlock(externalConnPos, Orientation(), LIGHT)) {
                logError("Couldn't insert light test circuit block", "CircuitTestGroup");
                return false;
            }

            const Block* block = blockContainer.getBlock(externalConnPos);
            if (!circuit->tryCreateConnection(ConnectionEnd(testedBlock->id(), iter->first), ConnectionEnd(block->id(), 0))) {
                logError("Couldn't create light test circuit connection, ext: {}", "CircuitTestGroup", externalConnPos);
                return false;
            }
            namePositionMap.insert({blockData->getConnectionIdToName(iter->first).value(), externalConnPos});
        }
    }
    for (auto iter = inputs.begin(); iter != inputs.end(); iter++) {
        if (namePositionMap.find(*iter) == namePositionMap.end()) {
            logWarning("Input '{}' expected by test not found on tested circuit, this may cause errors", "CircuitTestGroup", *iter);
        }
    }
    for (auto iter = outputs.begin(); iter != outputs.end(); iter++) {
        if (namePositionMap.find(*iter) == namePositionMap.end()) {
            logWarning("Output '{}' expected by test not found on tested circuit, this may cause errors", "CircuitTestGroup", *iter);
        }
    }
    return true;
}

bool CircuitTestGroup::runAllTests(BlockType blockType, bool haltOnFailure, simulator_id_t simulatorToUse) {
    std::vector<int> testIDs;
    for (int i =0; i < testCases.size(); i++) testIDs.push_back(i);
    return runTests(testIDs, blockType, haltOnFailure, simulatorToUse);
}

bool CircuitTestGroup::runTests(std::vector<std::string>& testsToRun, BlockType blockType, bool haltOnFailure, simulator_id_t simulatorToUse) {
    // converts strings to their IDs and runs them by ID
    std::vector<int> testIDs;
    for (auto stringIter = testsToRun.begin(); stringIter != testsToRun.end(); stringIter++) {
        auto idIter = testCaseNameToID.find(*stringIter);
        if (idIter == testCaseNameToID.end()) {
            logError("Unable to find test case with name '{}'", "CircuitTestGroup", *stringIter);
            return false;
        }
        testIDs.push_back(idIter->second);
    }
    return runTests(testIDs, blockType, haltOnFailure, simulatorToUse);
}

bool CircuitTestGroup::runTests(std::vector<int>& testsToRun, BlockType blockType, bool haltOnFailure, simulator_id_t simulatorToUse) {
    generateTestCircuit(blockType, simulatorToUse);
    bool fullTestSucceedStatus = true;

    for (auto testCaseIndexIter = testsToRun.begin(); testCaseIndexIter != testsToRun.end(); testCaseIndexIter++) {
        simulator->resetStates();
        simulator->setPause(true); //don't know if this is necessary but playing it safe
        logInfo("Running test case '{}'", "CircuitTestGroup", testCases[*testCaseIndexIter].name);
        for (auto commandIter = testCases[*testCaseIndexIter].testCommands.begin(); commandIter != testCases[*testCaseIndexIter].testCommands.end(); commandIter++) {
            logInfo("Performing a test command of type {}", "CircuitTestGroup", getTestCommandTypeString(commandIter->type));
            bool isTestGroupSuccessful = true;
            if (commandIter->type == NOP_COMMAND) {
                continue;
            } else if (commandIter->type == SET_STATES) {
                isTestGroupSuccessful = runSetStatesCommand(*commandIter, *simulator, namePositionMap);
            } else if (commandIter->type == CHECK_STATES) {
                isTestGroupSuccessful = runCheckStatesCommand(*commandIter, *simulator, namePositionMap);
            } else if (commandIter->type == TICK_STEP) {
                logInfo("Stepping forward by {} ticks", "CircuitTestGroup - TICK_STEP", commandIter->ticks);
                simulator->tickStep(commandIter->ticks);
            } else {
                logError("Unrecognized test command type", "CircuitTestGroup");
                isTestGroupSuccessful = false;
            }

            if (!isTestGroupSuccessful) {
                logError("Test group failed.", "CircuitTestGroup");
                fullTestSucceedStatus = false;
                if (haltOnFailure) {
                    return false;
                }
            }
        }
    }
    return fullTestSucceedStatus;
}

bool CircuitTestGroup::runSetStatesCommand(TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition) {
    for (auto statesIter = testCommand.states.begin(); statesIter != testCommand.states.end(); statesIter++) {
        auto blockPosIter = nameToConnectedBlockPosition.find(statesIter->first);
        if (blockPosIter == nameToConnectedBlockPosition.end()) {
            logError("Port '{}' does not match any on circuit", "CircuitTestGroup", statesIter->first);
            return false;
        }
        simulator.setState((Address(blockPosIter->second)), statesIter->second);
        logInfo("Set port '{}' to state '{}'", "CircuitTestGroup - SET_STATES", statesIter->first, statesIter->second);
    }
    return true;
}

bool CircuitTestGroup::runCheckStatesCommand(TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition) {
// probably needs to return the state of every single block tested instead of whether just one of them fails
    bool testSucceeded = true;
    for (auto statesIter = testCommand.states.begin(); statesIter != testCommand.states.end(); statesIter++) {
        auto blockPosIter = nameToConnectedBlockPosition.find(statesIter->first);
        if (blockPosIter == nameToConnectedBlockPosition.end()) {
            logError("Port '{}' does not match any on circuit", "CircuitTestGroup", statesIter->first);
            return false;
        }
        logic_state_t actualState = simulator.getState((Address(blockPosIter->second)));
        if (actualState != statesIter->second) {
            testSucceeded = false;
            logError("Inner test case failed: Expected port '{}' to have output '{}', got '{}'", "CircuitTestGroup - CHECK_STATES", statesIter->first, statesIter->second, actualState);
        } else {
            logInfo("Inner test case succeeded: Expected port '{}' to have output '{}', got '{}'", "CircuitTestGroup - CHECK_STATES", statesIter->first, statesIter->second, actualState);
        }
    }
    return testSucceeded;
}
