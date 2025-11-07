#ifndef circuitTestCase_h
#define circuitTestCase_h

#include "backend/evaluator/evalDefs.h"
#include "environment/environment.h"
#include "backend/evaluator/simulator/logicState.h"

class CircuitTestCase {
    typedef std::unordered_multimap<std::string, Position> NamePositionMap;
public:
    CircuitTestCase() {}
    bool runTest(BlockType blockType, bool haltOnFailure, Environment& environment);

private:
    enum TestCommandType {
        NOP_COMMAND,
        SET_STATES,
        CHECK_STATES,
        TICK_STEP
    };

    struct TestCommand {
        TestCommandType type = NOP_COMMAND;
        int ticks = 0;
        std::vector<std::pair<std::string, logic_state_t>> states = {};
    };

    void runSetStatesCommand(TestCommand testCommand, const SharedEvaluator eval, NamePositionMap& nameToConnectedBlockPosition);
    bool runCheckStatesCommand(TestCommand testCommand, const SharedEvaluator eval, NamePositionMap& nameToConnectedBlockPosition);

    std::vector<TestCommand> testCommands;
};

#endif