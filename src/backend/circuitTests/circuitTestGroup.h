#ifndef circuitTestGroup_h
#define circuitTestGroup_h

#include "backend/container/block/block.h"
#include "backend/container/block/blockDefs.h"
#include "backend/evaluator/evalDefs.h"
#include "backend/evaluator/simulator/logicState.h"

class Environment;
class EvalLogicSimulator;

class CircuitTestGroup {
    typedef std::unordered_multimap<std::string, Position> NamePositionMap;
public:
    CircuitTestGroup(std::string name) : name(name) {}
    std::string getName() {return name;}
    void addTestCase(std::string name);
    bool runAllTests(BlockType blockType, bool haltOnFailure, Environment& environment);
    bool runTests(std::vector<std::string>& testsToRun, BlockType blockType, bool haltOnFailure, Environment& environment);
    bool runTests(std::vector<int>& testsToRun, BlockType blockType, bool haltOnFailure, Environment& environment);
    bool addSetStatesCommand(std::string testCase, std::vector<std::pair<std::string, logic_state_t>> states);
    bool addCheckStatesCommand(std::string testCase, std::vector<std::pair<std::string, logic_state_t>> states);
    bool addTickStepCommand(std::string testCase, int ticks);
    

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

    struct TestCase {
        TestCase(const std::string& name, const std::vector<TestCommand>& testCommands = {}) : name(name), testCommands(testCommands) {}
        std::string name;
        std::vector<TestCommand> testCommands;
    };

    bool generateTestCircuit(BlockType blockType, Environment& environment);
    bool runSetStatesCommand(TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);
    bool runCheckStatesCommand(TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);

    std::string name;
    std::unordered_map<std::string, int> testCaseNameToID;
    std::vector<TestCase> testCases;
    NamePositionMap namePositionMap;
    EvalLogicSimulator* simulator;
};

#endif

// change things: make test cases as structs, make each test case have a name, have some info about whether it's a truth table