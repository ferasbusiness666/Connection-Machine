#include "saveTestCommand.h"

#include "environment/environment.h"
#include "util/runAtStartup.h"
#include "../commandManager.h"
#include "backend/circuitTests/circuitTestGroup.h"
#include "computerAPI/circuitTestFileManager.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<SaveTestCommand>());)

void SaveTestCommand::run(const std::vector<std::string>& args, Environment& environment) {
	if (args.size() != 3) {
		logError("Wrong number of arguments passed to save_test. Proper usage is 'save_test {test_name} {path}'", "SaveTestCommand");
		return;
	}

    CircuitTestGroupManager& testGroupManager = environment.getBackend().getCircuitTestGroupManager();
    CircuitTestGroup * testGroup = testGroupManager.getCircuitTestGroup(args[1]);
    if (testGroup == NULL) {
        logError("Invalid test group name {}.", "SaveTestCommand", args[1]);
        return;
    }

    if (!CircuitTestFileManager::saveToFile(args[2], *testGroup)) {
        logInfo("Not saved", "SaveTestCommand");
    } else {
        logInfo("Test Saved", "SaveTestCommand");
    }
}
