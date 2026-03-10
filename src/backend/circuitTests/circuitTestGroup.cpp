#include "circuitTestGroup.h"

#include "backend/address.h"
#include "backend/blockData/blockData.h"
#include "backend/circuit/circuit.h"
#include "backend/container/block/blockDefs.h"
#include "backend/evaluator/evaluator.h"
#include "backend/evaluator/simulator/evalLogicSimulator.h"
#include "backend/evaluator/simulator/logicState.h"
#include "backend/position/position.h"
#include "logging/logging.h"
#include "environment/environment.h"

const CircuitTestGroup::TestCase* CircuitTestGroup::getTestCase(int id) {
    if (id >= testCases.size()) return nullptr;
    return &testCases[id];
}

const CircuitTestGroup::TestCase* CircuitTestGroup::getTestCase(std::string name) {
    if (!testCaseNameToID.contains(name)) return nullptr;
    return &testCases[testCaseNameToID[name]];
}

void CircuitTestGroup::addTestCase(std::string name) {
    testCases.emplace_back(name);
    testCaseNameToID.emplace(std::make_pair(name, testCases.size()-1));
}

bool CircuitTestGroup::addSetStatesCommand(std::string testCase, std::vector<std::pair<std::string, logic_state_t>> states) {
    TestCommand testCommand = TestCommand(SET_STATES, 0, states);
    auto idIter = testCaseNameToID.find(testCase);
    if (idIter == testCaseNameToID.end()) {
        logError("Unrecognized test case {}", "CircuitTestGroup", testCase);
        return false;
    }
    testCases[idIter->second].testCommands.push_back(testCommand);
    return true;
}

bool CircuitTestGroup::addCheckStatesCommand(std::string testCase, std::vector<std::pair<std::string, logic_state_t>> states) {
    TestCommand testCommand = TestCommand(CHECK_STATES, 0, states);
    auto idIter = testCaseNameToID.find(testCase);
    if (idIter == testCaseNameToID.end()) {
        logError("Unrecognized test case {}", "CircuitTestGroup", testCase);
        return false;
    }
    testCases[idIter->second].testCommands.push_back(testCommand);
    return true;
}

bool CircuitTestGroup::addTickStepCommand(std::string testCase, int ticks) {
    TestCommand testCommand = TestCommand(TICK_STEP, ticks, {});
    auto idIter = testCaseNameToID.find(testCase);
    if (idIter == testCaseNameToID.end()) {
        logError("Unrecognized test case {}", "CircuitTestGroup", testCase);
        return false;
    }
    testCases[idIter->second].testCommands.push_back(testCommand);
    return true;
}

bool CircuitTestGroup::generateTestCircuit(BlockType blockType, Environment& environment) {
    Backend& backend = environment.getBackend();
    CircuitManager& cirManager = backend.getCircuitManager();
    SimulatorManager& evalManager = backend.getSimulatorManager();
    BlockDataManager& blockDataManager = backend.getBlockDataManager();

    circuit_id_t cirId = cirManager.createNewCircuit(false);
    SharedCircuit cir = cirManager.getCircuit(cirId);
    simulator_id_t simID = backend.createSimulator(cirId).value();
    simulator = evalManager.getSimulator(simID);
    simulator->setPause(true);
    const BlockContainer& blockContainer = cir->getBlockContainer();

    const BlockData* blockData = blockDataManager.getBlockData(blockType);
    std::unordered_map<connection_end_id_t, BlockData::ConnectionData> connections = blockData->getConnections();
    // change implementation of this when BlockData::getConnectionNameToId is implemented
    namePositionMap.clear();

    if (!cir->tryInsertBlock(Position(0,0), Orientation(), blockType)) {
        logError("Couldn't insert test circuit block {}", "circuitTestCase", "blockType");
        return false;
    }

	const Block* testedBlock = cir->getBlockContainer().getBlock(Position(0, 0));

    for (auto iter = connections.begin(); iter != connections.end(); iter++) {
        if (iter->second.portType == BlockData::ConnectionData::PortType::INPUT || iter->second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
            Position externalConnPos = Position(-1-namePositionMap.size(), 0);
            if (!cir->tryInsertBlock(externalConnPos, Orientation(), SWITCH)) {
                logError("Couldn't insert switch test circuit block", "circuitTestGroup");
                return false;
            }
            const Block* block = blockContainer.getBlock(externalConnPos);

            if (!cir->tryCreateConnection(ConnectionEnd(testedBlock->id(), iter->first), ConnectionEnd(block->id(), 0))) {
                logError("Couldn't create switch test circuit connection, ext: {}", "circuitTestGroup", externalConnPos);
                return false;
            }
            namePositionMap.insert({blockData->getConnectionIdToName(iter->first).value(), externalConnPos});
        }
        if (iter->second.portType == BlockData::ConnectionData::PortType::OUTPUT || iter->second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
            Position internalConnPos = Position(iter->second.positionOnBlock.dx, iter->second.positionOnBlock.dy);
            Position externalConnPos = Position(-1-namePositionMap.size(), 0);
            if (!cir->tryInsertBlock(externalConnPos, Orientation(), LIGHT)) {
                logError("Couldn't insert light test circuit block", "circuitTestGroup");
                return false;
            }
            const Block* block = blockContainer.getBlock(externalConnPos);

            if (!cir->tryCreateConnection(ConnectionEnd(testedBlock->id(), iter->first), ConnectionEnd(block->id(), 0))) {
                logError("Couldn't create light test circuit connection, ext: {} int: {}", "circuitTestGroup", externalConnPos, internalConnPos);
                return false;
            }
            namePositionMap.insert({blockData->getConnectionIdToName(iter->first).value(), externalConnPos});
        }
    }
    return true;
}

bool CircuitTestGroup::runAllTests(BlockType blockType, bool haltOnFailure, Environment& environment) {
    std::vector<int> testIDs;
    for (int i =0; i < testCases.size(); i++) testIDs.push_back(i);
    return runTests(testIDs, blockType, haltOnFailure, environment);
}

bool CircuitTestGroup::runTests(std::vector<std::string>& testsToRun, BlockType blockType, bool haltOnFailure, Environment& environment) {
    // converts strings to their IDs and runs them by ID
    std::vector<int> testIDs;
    for (auto stringIter = testsToRun.begin(); stringIter != testsToRun.end(); stringIter++) {
        auto idIter = testCaseNameToID.find(*stringIter);
        if (idIter == testCaseNameToID.end()) {
            logError("Unrecognized test case {}", "CircuitTestGroup", *stringIter);
            return false;
        }
        testIDs.push_back(idIter->second);
    }
    return runTests(testIDs, blockType, haltOnFailure, environment);
}

bool CircuitTestGroup::runTests(std::vector<int>& testsToRun, BlockType blockType, bool haltOnFailure, Environment& environment) {
    generateTestCircuit(blockType, environment);
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
                logError("Unrecognized test command", "CircuitTestGroup");
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
            logError("Port {} does not match any on circuit", "CircuitTestGroup", statesIter->first);
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
            logError("Port {} does not match any on circuit", "CircuitTestGroup", statesIter->first);
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


/*
1. Test if a block type behaves in the way the test expects
2. The tested circuit will be abstracted into a block that we run tests on through the ports.

1. Make a circuit, place the abstracted block at (0,0), run tests on it
2. To get connection end ID from port name, use connectionIdNames map from blockData
3. To get port position, use getConnectionVector
4. Create eval for circuit if not already there
5. Create things (switch & lights) connected to ports of the circuit block to evaluate it
6. Create connections with circuit createConnection
7. Use stuff in circuit.h to create blocks (tryInsertBlock, tryCreateConnection)
8. Create new circuit using circuitManager createNewCircuit
*/