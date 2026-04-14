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
    CircuitTestGroupRunner(Backend& backend, std::string name, BlockType blockType) :
		name(name), backend(backend), blockType(blockType) {}
    std::string getName() const {return name;}

    const CircuitTestGroup* getCircuitTestGroup();

    bool runAllTests(bool haltOnFailure);
    bool runTests(std::vector<std::string>& testsToRun, bool haltOnFailure);
    bool runTests(std::vector<int>& testsToRun, bool haltOnFailure);

private:
    bool generateTestCircuit();
    bool runSetStatesCommand(CircuitTestGroup::TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);
    bool runCheckStatesCommand(CircuitTestGroup::TestCommand testCommand, EvalLogicSimulator& simulator, NamePositionMap& nameToConnectedBlockPosition);

    std::string name;
    circuit_id_t circuitID;
    simulator_id_t simID;
    NamePositionMap namePositionMap;
    EvalLogicSimulator* simulator;
	Backend& backend;
    BlockType blockType;
};

#endif /* circuitTestGroupRunner_h */
