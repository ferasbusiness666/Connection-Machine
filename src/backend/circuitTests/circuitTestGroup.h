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
    enum TestCommandType {
        NOP_COMMAND,
        SET_STATES,
        CHECK_STATES,
        TICK_STEP
    };

    static std::string getTestCommandTypeString(TestCommandType type) {
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

    CircuitTestGroup(std::string name) : name(name) {}
    void addTestCase(std::string name);
    bool addInput(std::string input);
    bool addOutput(std::string output);
    bool addSetStatesCommand(std::string testCase, std::vector<std::pair<std::string, logic_state_t>> states);
    bool addCheckStatesCommand(std::string testCase, std::vector<std::pair<std::string, logic_state_t>> states);
    bool addTickStepCommand(std::string testCase, int ticks);
    std::string getName() {return name;}
    const TestCase* getTestCase(int id);
    const TestCase* getTestCase(std::string name);
    std::set<std::string>::const_iterator getInputIterator() {return inputs.cbegin();} // TODO: is this okay?
    std::set<std::string>::const_iterator getOutputIterator() {return outputs.cbegin();}
    std::set<std::string>::const_iterator getInputIteratorEnd() {return inputs.cend();}
    std::set<std::string>::const_iterator getOutputIteratorEnd() {return outputs.cend();}
    bool runAllTests(BlockType blockType, bool haltOnFailure, Environment& environment);
    bool runTests(std::vector<std::string>& testsToRun, BlockType blockType, bool haltOnFailure, Environment& environment);
    bool runTests(std::vector<int>& testsToRun, BlockType blockType, bool haltOnFailure, Environment& environment);

private:
    bool generateTestCircuit(BlockType blockType, Environment& environment);
    bool runSetStatesCommand(TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);
    bool runCheckStatesCommand(TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);

    bool isTruthTable = false;
    std::string name;
    std::unordered_map<std::string, int> testCaseNameToID;
    std::vector<TestCase> testCases;
    NamePositionMap namePositionMap;
    EvalLogicSimulator* simulator;
    std::set<std::string> inputs; // TODO: actually make use of these somehow
    std::set<std::string> outputs;

};

#endif

// change things: make test cases as structs, make each test case have a name, have some info about whether it's a truth table