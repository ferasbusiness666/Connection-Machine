#ifndef circuitTestGroupRunner_h
#define circuitTestGroupRunner_h

#include "backend/circuit/circuitDefs.h"
#include "backend/container/block/blockDefs.h"
#include "backend/evaluator/evalDefs.h"
#include "backend/evaluator/simulator/logicState.h"
#include "backend/circuitTests/circuitTestGroup.h"

class Backend;
class EvalLogicSimulator;

class CircuitTestGroupRunner {
    typedef std::unordered_multimap<std::string, Position> NamePositionMap;
public:
    enum TestResult {
        NOT_RUN,
        SUCCEEDED,
        FAILED,
        ERROR
    };

    struct TestRunData {
        TestRunData(const TestResult& result = NOT_RUN, const std::string& message = "No message.", const circuit_id_t circuitID = 0) : result(result), message(message), circuitID(circuitID) {}
        TestResult result = NOT_RUN;
        std::string message = "No message.";
        circuit_id_t circuitID = 0;
    };

    CircuitTestGroupRunner(Backend& backend, std::string name, BlockType blockType) :
		name(name), backend(backend), blockType(blockType) {generateTestCircuit();}
    std::string getName() const {return name;}

    const CircuitTestGroup* getCircuitTestGroup();

    bool runAllTests(bool haltOnFailure);
    bool runTests(const std::vector<std::string>& testsToRun, bool haltOnFailure);
    bool runTests(const std::vector<int>& testsToRun, bool haltOnFailure);
    simulator_id_t getSimulatorId() const {return simID;}
    circuit_id_t getCircuitId() const {return circuitID;}
private:
    bool generateTestCircuit();
    bool runSetStatesCommand(CircuitTestGroup::TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);
    bool runCheckStatesCommand(CircuitTestGroup::TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);
    void sendTestResultUpdate(int testCaseID);

    std::string name;
    circuit_id_t circuitID;
    simulator_id_t simID;
    NamePositionMap namePositionMap;
    EvalLogicSimulator* simulator;
	Backend& backend;
    BlockType blockType;
};

#endif /* circuitTestGroupRunner_h */
