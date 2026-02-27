#include "runTestCommand.h"

#include "backend/circuit/circuit.h"
#include "environment/environment.h"
#include "util/runAtStartup.h"
#include "../commandManager.h"
#include "backend/circuitTests/circuitTestGroup.h"
#include "backend/container/block/blockDefs.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<RunTestCommand>());)

void RunTestCommand::run(const std::vector<std::string>& args, Environment& environment) {
	if (args.size() != 3) {
		logError("Wrong number of arguments passed to test_circuit. Proper usage is 'test_circuit {test_id} {circuit_id}'", "RunTestCommand");
		return;
	}

    circuit_id_t cirID;
    try {
        cirID = std::stoi(args[2]);
    }
    catch (...) {
        logError("Exception occured. Check the circuit_id parameter, it should be a reasonably-sized integer.", "RunTestCommand");
        return;
    }
    
    BlockType circuitBlock = environment.getBackend().getCircuitManager().setupBlockData(cirID);
    if (circuitBlock == 0) {
        logError("Invalid circuit ID. Run list_circuits to get a list of circuit IDs.", "RunTestCommand");
        return;
    }

    CircuitTestGroupManager& testGroupManager = environment.getBackend().getCircuitTestGroupManager();
    CircuitTestGroup * testCase = testGroupManager.getCircuitTestGroup(args[1]);
    if (testCase == NULL) {
        logError("Invalid test group name {}.", "RunTestCommand", args[1]);
        return;
    }

    if (!testCase->runAllTests(circuitBlock, false, environment)) {
        logInfo("Test failed.", "RunTestCommand");
    }
    else {
        logInfo("Test succeeded.", "RunTestCommand");
    }
}
