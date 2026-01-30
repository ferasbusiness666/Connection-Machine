#include "testCircuitCommand.h"

#include "util/runAtStartup.h"
#include "../commandManager.h"
#include "backend/circuitTestCase/circuitTestCase.h"
#include "backend/container/block/blockDefs.h"
#include "computerAPI/testCaseFileManager.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<TestCircuitCommand>());)

void TestCircuitCommand::run(const std::vector<std::string>& args, Environment& environment) {
	if (args.size() != 3) {
		logError("Wrong number of arguments passed to test_circuit. Proper usage is 'test_circuit {test_file_path} {circuit_id}'", "TestCircuitCommand");
		return;
	}

    circuit_id_t cirID;
    try {
        cirID = std::stoi(args[2]);
    }
    catch (...) {
        logError("Exception occured. Check the circuit_id parameter, it should be a reasonably-sized integer.", "GetBlockDataCommand");
        return;
    }
    
    BlockType circuitBlock = environment.getBackend().getCircuitManager().setupBlockData(cirID);

    std::optional<CircuitTestCase> testCase = TestCaseFileManager::getCircuitTestCaseFromFilePath(args[1]);
    if (testCase == std::nullopt) {
        logInfo("No tests run", "TestCircuitCommand");
        return;
    }
    if (!testCase.value().runTest(circuitBlock, false, environment)) {
        logInfo("Test case failed.", "TestCircuitCommand");
    }
    else {
        logInfo("Test case succeeded.", "TestCircuitCommand");
    }
}
