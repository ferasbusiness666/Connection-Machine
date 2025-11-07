#include "circuitTestCase.h"

#include "backend/address.h"
#include "backend/blockData/blockData.h"
#include "backend/evaluator/evalDefs.h"
#include "backend/evaluator/evaluator.h"
#include "backend/position/position.h"

bool CircuitTestCase::runTest(BlockType blockType, bool haltOnFailure, Environment& environment) {
    Backend& backend = environment.getBackend();
    CircuitManager& cirManager = backend.getCircuitManager();
    const EvaluatorManager& evalManager = backend.getEvaluatorManager();
    BlockDataManager& blockDataManager = backend.getBlockDataManager();

    circuit_id_t cirId = cirManager.createNewCircuit(false);
    SharedCircuit cir = cirManager.getCircuit(cirId);
    evaluator_id_t evalId = backend.createEvaluator(cirId).value();
    const SharedEvaluator eval = evalManager.getEvaluator(evalId);
    eval->setPause(true);
    const BlockContainer blockContainer = cir->getBlockContainer();

    if (!cir->tryInsertBlock(Position(0,0), Orientation(), blockType)) {
        logError("Couldn't insert test circuit block {}", "circuitTestCase", "blockType");
        return false;
    }

    const BlockData* blockData = blockDataManager.getBlockData(blockType);
    std::unordered_map<connection_end_id_t, BlockData::ConnectionData> connections = blockData->getConnections();
    // change implementation of this when BlockData::getConnectionNameToId is implemented
    NamePositionMap nameToConnectedBlockPosition;

    for (auto iter = connections.begin(); iter != connections.end(); iter++) {
        if (iter->second.portType == BlockData::ConnectionData::PortType::INPUT || iter->second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
            Position internalConnPos = Position(iter->second.positionOnBlock.dx, iter->second.positionOnBlock.dy);
            Position externalConnPos = Position(-1-nameToConnectedBlockPosition.size(), 0);
            if (!cir->tryInsertBlock(externalConnPos, Orientation(), SWITCH)) {
                logError("Couldn't insert test circuit block", "circuitTestCase");
                return false;
            }
            if (!cir->tryCreateConnection(externalConnPos, internalConnPos)) {
                logError("Couldn't create test circuit connection", "circuitTestCase");
                return false;
            }
            std::optional<std::string> test = blockData->getConnectionIdToName(iter->first);
            if (test == std::nullopt) {
                logError("Uh oh i'm gonna explode!!! {} not in gcitn", "CircuitTestCase", iter->first);
            }
            nameToConnectedBlockPosition.insert({blockData->getConnectionIdToName(iter->first).value(), externalConnPos});
        }
        if (iter->second.portType == BlockData::ConnectionData::PortType::OUTPUT || iter->second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
            Position internalConnPos = Position(iter->second.positionOnBlock.dx, iter->second.positionOnBlock.dy);
            Position externalConnPos = Position(-1-nameToConnectedBlockPosition.size(), 0);
            if (!cir->tryInsertBlock(externalConnPos, Orientation(), LIGHT)) {
                logError("Couldn't insert test circuit block", "circuitTestCase");
                return false;
            }
            if (!cir->tryCreateConnection(internalConnPos, externalConnPos)) {
                logError("Couldn't create test circuit connection", "circuitTestCase");
                return false;
            }
            std::optional<std::string> test = blockData->getConnectionIdToName(iter->first);
            if (test == std::nullopt) {
                logError("Uh oh i'm gonna explode!!! {} not in gcitn", "CircuitTestCase", iter->first);
            }
            nameToConnectedBlockPosition.insert({blockData->getConnectionIdToName(iter->first).value(), externalConnPos});
        }
    }
    for (auto iter = nameToConnectedBlockPosition.begin(); iter != nameToConnectedBlockPosition.end(); iter++) {
        logInfo("Block {} is in pos {}", "CircuitTestCase", iter->first, iter->second);
    }

    for (auto commandIter = testCommands.begin(); commandIter != testCommands.end(); commandIter++) {
        if (commandIter->type == NOP_COMMAND) {
            continue;
        } else if (commandIter->type == SET_STATES) {
            runSetStatesCommand(*commandIter, eval, nameToConnectedBlockPosition);
        } else if (commandIter->type == CHECK_STATES) {
            continue;
        } else if (commandIter->type == TICK_STEP) {
            continue;
        } else {
            logError("Unrecognized test command", "CircuitTestCase");
        }
    }
    return true;
}

bool CircuitTestCase::runSetStatesCommand(TestCommand testCommand, const SharedEvaluator eval, NamePositionMap& nameToConnectedBlockPosition) {
    for (auto statesIter = testCommand.states.begin(); statesIter != testCommand.states.end(); statesIter++) {
        auto blockPosIter = nameToConnectedBlockPosition.find(statesIter->first);
        eval->setState((Address(blockPosIter->second)), statesIter->second);
    } 
}

bool CircuitTestCase::runCheckStatesCommand(TestCommand testCommand, const SharedEvaluator eval, NamePositionMap&) {

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