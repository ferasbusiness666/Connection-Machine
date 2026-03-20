#ifndef circuitTestGroup_h
#define circuitTestGroup_h

#include "backend/container/block/block.h"
#include "backend/container/block/blockDefs.h"
#include "backend/evaluator/evalDefs.h"
#include "backend/evaluator/simulator/logicState.h"
#include <utility>

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
        TestCommand(const TestCommandType& type = NOP_COMMAND, const int& ticks = 0, const std::vector<std::pair<std::string, logic_state_t>>& states = {}) : type(type), ticks(ticks), states(states) {}
        TestCommandType type = NOP_COMMAND;
        int ticks = 0;
        std::vector<std::pair<std::string, logic_state_t>> states = {};
    };

    struct TestCase {
        TestCase(const std::string& name, const std::vector<TestCommand>& testCommands = {}) : name(name), testCommands(testCommands) {}
        std::string name;
        std::vector<TestCommand> testCommands;
    };

    CircuitTestGroup(std::string name, bool isTruthTable, int truthTableTicks) : name(name), isTruthTable(isTruthTable), truthTableTicks(truthTableTicks) {}
    bool addTestCase(std::string name);
    bool addInput(std::string input);
    bool addOutput(std::string output);
    bool addSetStatesCommand(std::string testCase, std::vector<std::pair<std::string, logic_state_t>> states);
    bool addCheckStatesCommand(std::string testCase, std::vector<std::pair<std::string, logic_state_t>> states);
    bool addTickStepCommand(std::string testCase, int ticks);
    bool addSimpleTestCase(std::string name, std::vector<std::pair<std::string, logic_state_t>> inputStates, std::vector<std::pair<std::string, logic_state_t>> outputStates);
    std::string getName() {return name;}
    const TestCase* getTestCase(int id);
    const TestCase* getTestCase(std::string name);
    int getTruthTableTicks() {return truthTableTicks;}
    bool truthTable() {return isTruthTable;}
    std::vector<std::string>::const_iterator getInputIterator() {return inputs.cbegin();}
    std::vector<std::string>::const_iterator getOutputIterator() {return outputs.cbegin();}
    std::vector<std::string>::const_iterator getInputIteratorEnd() {return inputs.cend();}
    std::vector<std::string>::const_iterator getOutputIteratorEnd() {return outputs.cend();}
    bool runAllTests(BlockType blockType, bool haltOnFailure, Environment& environment);
    bool runTests(std::vector<std::string>& testsToRun, BlockType blockType, bool haltOnFailure, Environment& environment);
    bool runTests(std::vector<int>& testsToRun, BlockType blockType, bool haltOnFailure, Environment& environment);

private:
    bool generateTestCircuit(BlockType blockType, Environment& environment);
    bool runSetStatesCommand(TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);
    bool runCheckStatesCommand(TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);

    std::string name;
    // truth tables follow a strict format of every test case having a set state, a tick step (universal across all test cases,
    // stored in the truthTableTicks value), and a get state. it only allows adding commands via the addSimpleTestCase method.
    bool isTruthTable;
    int truthTableTicks; // setting this to -1 should make the timing automatic
    std::unordered_map<std::string, int> testCaseNameToID;
    std::vector<TestCase> testCases;
    NamePositionMap namePositionMap;
    EvalLogicSimulator* simulator;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;

};

#endif