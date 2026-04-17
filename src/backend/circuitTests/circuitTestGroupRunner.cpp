#include "circuitTestGroupRunner.h"

#include "backend/circuit/circuitDefs.h"
#include "backend/circuitTests/circuitTestGroup.h"
#include "backend/container/block/block.h"
#include "backend/evaluator/simulator/evalLogicSimulator.h"
#include "backend/evaluator/simulatorManager.h"
#include "backend/backend.h"
#include <cassert>
#include <utility>

const CircuitTestGroup* CircuitTestGroupRunner::getCircuitTestGroup() {
    return backend.getCircuitTestGroupManager().getCircuitTestGroup(name);
}

void CircuitTestGroupRunner::sendTestResultUpdate(int testCaseID) {
    // std::get<n>(tuple)
    std::tuple<std::string, circuit_id_t, int> values = std::make_tuple(name, circuitID, testCaseID);
    backend.getDataUpdateEventManager().sendEvent("testRunDataUpdate", values);
}

bool CircuitTestGroupRunner::generateTestCircuit() {
    if (blockType == BlockType::NONE) {
        circuitID = 0;
        simID = 0;
        return false;
    }
    const CircuitTestGroup* testGroupData = getCircuitTestGroup();
    CircuitManager& cirManager = backend.getCircuitManager();
    SimulatorManager& evalManager = backend.getSimulatorManager();
    BlockDataManager& blockDataManager = backend.getBlockDataManager();

    circuitID = cirManager.createNewCircuit(false);
	simID = backend.createSimulator(circuitID).value();
    simulator = evalManager.getSimulator(simID);
    simulator->setPause(true);
    Circuit* circuit = cirManager.getCircuit(circuitID);
    circuit->clear();
    circuit->setEditable(false);
    BlockData* circuitBlockData = blockDataManager.getBlockData(circuit->getBlockType());
    assert(circuitBlockData);
    circuitBlockData->setIsPlaceable(false);

    const BlockContainer& blockContainer = circuit->getBlockContainer();

    const BlockData* blockData = blockDataManager.getBlockData(blockType);
    std::unordered_map<connection_end_id_t, BlockData::ConnectionData> connections = blockData->getConnections();
    // change implementation of this when BlockData::getConnectionNameToId is implemented
    namePositionMap.clear();

    if (!circuit->tryInsertBlock(Position(0,0), Orientation(), blockType)) {
        logError("Couldn't insert test circuit block '{}'", "circuitTestGroupRunner", blockType);
        return false;
    }

	const Block* testedBlock = circuit->getBlockContainer().getBlock(Position(0, 0));

    // iterate through the connections on the block type and create a switch/light to represent each input/output on a test circuit
    for (auto iter = connections.begin(); iter != connections.end(); iter++) {
        if (iter->second.portType == BlockData::ConnectionData::PortType::INPUT || iter->second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
            Position externalConnPos = Position(-1-namePositionMap.size(), 0);

            std::optional<std::string> blockNameOpt = blockData->getConnectionIdToName(iter->first);
            if (blockNameOpt == std::nullopt) {
                logError("Unable to resolve block name for id {}", "circuitTestGroupRunner", iter->first);
                return false;
            }
            std::string blockName = blockNameOpt.value();
            if (std::find(testGroupData->inputs.begin(), testGroupData->inputs.end(), blockName) == testGroupData->inputs.end()) {
                logWarning("Tested circuit's port '{}' does not match any inputs expected by test, this may cause errors", "circuitTestGroupRunner", blockName);
            }
            if (!circuit->tryInsertBlock(externalConnPos, Orientation(), SWITCH)) {
                logError("Couldn't insert switch test circuit block", "circuitTestGroupRunner");
                return false;
            }

            const Block* block = blockContainer.getBlock(externalConnPos);
            if (!circuit->tryCreateConnection(ConnectionEnd(testedBlock->id(), iter->first), ConnectionEnd(block->id(), 0))) {
                logError("Couldn't create switch test circuit connection, ext: {}", "circuitTestGroupRunner", externalConnPos);
                return false;
            }
            namePositionMap.insert({blockData->getConnectionIdToName(iter->first).value(), externalConnPos});
        }
        if (iter->second.portType == BlockData::ConnectionData::PortType::OUTPUT || iter->second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
            Position externalConnPos = Position(-1-namePositionMap.size(), 0);

            std::optional<std::string> blockNameOpt = blockData->getConnectionIdToName(iter->first);
            if (blockNameOpt == std::nullopt) {
                logError("Unable to resolve block name for id {}", "circuitTestGroupRunner", iter->first);
                return false;
            }
            std::string blockName = blockNameOpt.value();
            if (std::find(testGroupData->outputs.begin(), testGroupData->outputs.end(), blockName) == testGroupData->outputs.end()) {
                logWarning("Tested circuit's port '{}' does not match any outputs expected by test, this may cause errors", "circuitTestGroupRunner", blockName);
            }
            if (!circuit->tryInsertBlock(externalConnPos, Orientation(), LIGHT)) {
                logError("Couldn't insert light test circuit block", "circuitTestGroupRunner");
                return false;
            }

            const Block* block = blockContainer.getBlock(externalConnPos);
            if (!circuit->tryCreateConnection(ConnectionEnd(testedBlock->id(), iter->first), ConnectionEnd(block->id(), 0))) {
                logError("Couldn't create light test circuit connection, ext: {}", "circuitTestGroupRunner", externalConnPos);
                return false;
            }
            namePositionMap.insert({blockData->getConnectionIdToName(iter->first).value(), externalConnPos});
        }
    }
    for (auto iter = testGroupData->inputs.begin(); iter != testGroupData->inputs.end(); iter++) {
        if (namePositionMap.find(*iter) == namePositionMap.end()) {
            logWarning("Input '{}' expected by test not found on tested circuit, this may cause errors", "circuitTestGroupRunner", *iter);
        }
    }
    for (auto iter = testGroupData->outputs.begin(); iter != testGroupData->outputs.end(); iter++) {
        if (namePositionMap.find(*iter) == namePositionMap.end()) {
            logWarning("Output '{}' expected by test not found on tested circuit, this may cause errors", "circuitTestGroupRunner", *iter);
        }
    }
    return true;
}

bool CircuitTestGroupRunner::runAllTests(bool haltOnFailure) {
    const CircuitTestGroup* testGroupData = getCircuitTestGroup();
    std::vector<int> testIDs;
    for (int i =0; i < testGroupData->testCases.size(); i++) testIDs.push_back(i);
    return runTests(testIDs, haltOnFailure);
}

bool CircuitTestGroupRunner::runTests(const std::vector<std::string>& testsToRun, bool haltOnFailure) {
    const CircuitTestGroup* testGroupData = getCircuitTestGroup();
    // converts strings to their IDs and runs them by ID
    std::vector<int> testIDs;
    for (auto stringIter = testsToRun.begin(); stringIter != testsToRun.end(); stringIter++) {
        auto idIter = testGroupData->testCaseNameToID.find(*stringIter);
        if (idIter == testGroupData->testCaseNameToID.end()) {
            logError("Unable to find test case with name '{}'", "circuitTestGroupRunner", *stringIter);
            return false;
        }
        testIDs.push_back(idIter->second);
    }
    return runTests(testIDs, haltOnFailure);
}

bool CircuitTestGroupRunner::runTests(const std::vector<int>& testsToRun, bool haltOnFailure) {
    const CircuitTestGroup* testGroupData = getCircuitTestGroup();
    bool fullTestSucceedStatus = true;

    for (auto testCaseIndexIter = testsToRun.begin(); testCaseIndexIter != testsToRun.end(); testCaseIndexIter++) {
        simulator->resetStates();
        simulator->setPause(true); //don't know if this is necessary but playing it safe
        logInfo("Running test case '{}'", "circuitTestGroupRunner", testGroupData->testCases[*testCaseIndexIter].name);
        for (auto commandIter = testGroupData->testCases[*testCaseIndexIter].testCommands.begin(); commandIter != testGroupData->testCases[*testCaseIndexIter].testCommands.end(); commandIter++) {
            logInfo("Performing a test command of type {}", "circuitTestGroupRunner", CircuitTestGroup::getTestCommandTypeString(commandIter->type));
            bool isTestGroupSuccessful = true;
            if (commandIter->type == CircuitTestGroup::NOP_COMMAND) {
                continue;
            } else if (commandIter->type == CircuitTestGroup::SET_STATES) {
                isTestGroupSuccessful = runSetStatesCommand(*commandIter, *simulator, namePositionMap);
            } else if (commandIter->type == CircuitTestGroup::CHECK_STATES) {
                isTestGroupSuccessful = runCheckStatesCommand(*commandIter, *simulator, namePositionMap);
            } else if (commandIter->type == CircuitTestGroup::TICK_STEP) {
                logInfo("Stepping forward by {} ticks", "circuitTestGroupRunner - TICK_STEP", commandIter->ticks);
                simulator->tickStep(commandIter->ticks);
            } else {
                logError("Unrecognized test command type", "circuitTestGroupRunner");
                isTestGroupSuccessful = false;
            }

            if (!isTestGroupSuccessful) {
                logError("Test group failed.", "circuitTestGroupRunner");
                fullTestSucceedStatus = false;
                if (haltOnFailure) {
                    return false;
                }
            }
        }
    }
    return fullTestSucceedStatus;
}

bool CircuitTestGroupRunner::runSetStatesCommand(CircuitTestGroup::TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition) {
    for (auto statesIter = testCommand.states.begin(); statesIter != testCommand.states.end(); statesIter++) {
        auto blockPosIter = nameToConnectedBlockPosition.find(statesIter->first);
        if (blockPosIter == nameToConnectedBlockPosition.end()) {
            logError("Port '{}' does not match any on circuit", "circuitTestGroupRunner", statesIter->first);
            return false;
        }
        simulator.setState((Address(blockPosIter->second)), statesIter->second);
        logInfo("Set port '{}' to state '{}'", "circuitTestGroupRunner - SET_STATES", statesIter->first, statesIter->second);
    }
    return true;
}

bool CircuitTestGroupRunner::runCheckStatesCommand(CircuitTestGroup::TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition) {
// probably needs to return the state of every single block tested instead of whether just one of them fails
    bool testSucceeded = true;
    for (auto statesIter = testCommand.states.begin(); statesIter != testCommand.states.end(); statesIter++) {
        auto blockPosIter = nameToConnectedBlockPosition.find(statesIter->first);
        if (blockPosIter == nameToConnectedBlockPosition.end()) {
            logError("Port '{}' does not match any on circuit", "circuitTestGroupRunner", statesIter->first);
            return false;
        }
        logic_state_t actualState = simulator.getState((Address(blockPosIter->second)));
        if (actualState != statesIter->second) {
            testSucceeded = false;
            logError("Inner test case failed: Expected port '{}' to have output '{}', got '{}'", "circuitTestGroupRunner - CHECK_STATES", statesIter->first, statesIter->second, actualState);
        } else {
            logInfo("Inner test case succeeded: Expected port '{}' to have output '{}', got '{}'", "circuitTestGroupRunner - CHECK_STATES", statesIter->first, statesIter->second, actualState);
        }
    }
    return testSucceeded;
}
