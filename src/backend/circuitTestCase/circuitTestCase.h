#ifndef circuitTestCase_h
#define circuitTestCase_h

#include "environment/environment.h"
#include "backend/evaluator/simulator/logicState.h"

class CircuitTestCase {
public:
    CircuitTestCase() {}
    bool runTest(BlockType blockType, bool haltOnFailure, Environment& environment);

private:
    enum TestCommandType {
        GENERIC_COMMAND,
        SET_STATES,
        CHECK_STATES,
        TICK_STEP
    };

    struct TestCommand {
        TestCommandType type = GENERIC_COMMAND;
        int ticks = 0;
        std::vector<std::pair<std::string, logic_state_t>> states = {};
    };

    std::vector<TestCommand> testCommands;
};

#endif