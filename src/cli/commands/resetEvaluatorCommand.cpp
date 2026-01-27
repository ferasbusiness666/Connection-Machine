/*#include "resetSimulatoruatorCommand.h"

#include "environment/environment.h"
#include "../commandManager.h"
#include "util/runAtStartup.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<ResetSimulatoruatorCommand>());)

void ResetSimulatoruatorCommand::run(const std::vector<std::string>& args, Environment& environment) {
	if (args.size() != 2) {
		logError("Wrong number of arguments passed to reset_simulator. Proper usage is 'reset_simulator {eval_id}'", "ResetSimulatoruatorCommand");
		return;
	}

    int simulatorId;
    try {
        simulatorId = std::stoi(args[1]);
    }
    catch (...) {
        logError("Exception occured. Check your arguments, they should be reasonably-sized integers.", "ResetSimulatoruatorCommand");
        return;
    }
    const EvalLogicSimulator* eval = environment.getBackend().getSimulatorManager().getSimulator(simulatorId);
    if (eval == nullptr) {
        logError("Unrecognized simulator ID. Available simulators can be found with the 'list_simulators' command.", "ResetSimulatoruatorCommand");
        return;
    }
    eval->reset();
    logInfo("Reset simulator of ID {}", "ResetSimulatoruatorCommand", simulatorId);
}
*/