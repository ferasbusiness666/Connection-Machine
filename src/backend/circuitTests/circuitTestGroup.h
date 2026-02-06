#ifndef circuitTestGroup_h
#define circuitTestGroup_h

#include "backend/evaluator/evalDefs.h"
#include "environment/environment.h"
#include "backend/evaluator/simulator/logicState.h"

// add upper level logic later
class CircuitTestGroup {
    typedef std::unordered_multimap<std::string, Position> NamePositionMap;
public:
    CircuitTestGroup() {}
    bool runTest(BlockType blockType, bool haltOnFailure, Environment& environment);
    void addSetStatesCommand(std::vector<std::pair<std::string, logic_state_t>> states);
    void addCheckStatesCommand(std::vector<std::pair<std::string, logic_state_t>> states);
    void addTickStepCommand(int ticks);

private:
    enum TestCommandType {
        NOP_COMMAND,
        SET_STATES,
        CHECK_STATES,
        TICK_STEP
    };

    std::string getTestCommandTypeString(TestCommandType type) {
        switch (type) {
            case 0:
                return "NOP_COMMAND";
            case 1:
                return "SET_STATES";
            case 2:
                return "CHECK_STATES";
            case 3:
                return "TICK_STEP";
            default:
                return "UNKNOWN";
       }
    }


    struct TestCommand {
        TestCommandType type = NOP_COMMAND;
        int ticks = 0;
        std::vector<std::pair<std::string, logic_state_t>> states = {};
    };

    void generateTestCircuit();
    void runSetStatesCommand(TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);
    bool runCheckStatesCommand(TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);

    std::vector<TestCommand> testCommands;
};

#endif

// change things: make groups as structs, make each group have a name, have some info about whether it's a truth table