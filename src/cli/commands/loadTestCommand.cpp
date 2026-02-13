#include "loadTestCommand.h"

#include "backend/circuit/circuit.h"
#include "util/runAtStartup.h"
#include "../commandManager.h"
#include "backend/circuitTests/circuitTestGroup.h"
#include "backend/container/block/blockDefs.h"
#include "computerAPI/circuitTestFileManager.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<LoadTestCommand>());)

void LoadTestCommand::run(const std::vector<std::string>& args, Environment& environment) {
	if (args.size() != 3) {
		logError("Wrong number of arguments passed to test_circuit. Proper usage is 'test_circuit {test_file_path} {circuit_id}'", "LoadTestCommand");
		return;
	}

    circuit_id_t cirID;
    try {
        cirID = std::stoi(args[2]);
    }
    catch (...) {
        logError("Exception occured. Check the circuit_id parameter, it should be a reasonably-sized integer.", "LoadTestCommand");
        return;
    }
    
    BlockType circuitBlock = environment.getBackend().getCircuitManager().setupBlockData(cirID);
    if (circuitBlock == 0) {
        logError("Invalid circuit ID. Run list_circuits to get a list of circuit IDs.", "LoadTestCommand");
        return;
    }

    std::optional<CircuitTestGroup> testCase = CircuitTestFileManager::getCircuitTestFromFilePath(args[1]);
    if (testCase == std::nullopt) {
        logInfo("No tests run", "LoadTestCommand");
        return;
    }

    if (!testCase.value().runAllTests(circuitBlock, false, environment)) {
        logInfo("Test failed.", "LoadTestCommand");
    }
    else {
        logInfo("Test succeeded.", "LoadTestCommand");
    }
}
