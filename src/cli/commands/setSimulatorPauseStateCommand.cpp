#include "setSimulatorPauseStateCommand.h"

#include "backend/evaluator/simulator/evalLogicSimulator.h"
#include "environment/environment.h"
#include "util/runAtStartup.h"
#include "../commandManager.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<SetSimulatorPauseStateCommand>());)

void SetSimulatorPauseStateCommand::run(const std::vector<std::string>& args, Environment& environment) {
	if (args.size() != 3) {
		logError("Wrong number of arguments passed to set_eval_pause_state. Proper usage is 'set_eval_pause_state {eval_id} {state}'", "SetSimulatorPauseStateCommand");
		return;
	}

    int simulatorId;
    int state;
    try {
        simulatorId = std::stoi(args[1]);
        state = std::stoi(args[2]);
    }
    catch (...) {
        logError("Exception occured. Check your arguments, they should be reasonably-sized integers.", "SetSimulatorPauseStateCommand");
        return;
    }
    if (state < 0 || state > 1) {
        logError("Invalid state. Valid states are 0 for unpaused, 1 for paused.", "SetSimulatorPauseStateCommand");
        return;
    }
    EvalLogicSimulator* simulator = environment.getBackend().getSimulatorManager().getSimulator(simulator_id_t(simulatorId));
    if (simulator == nullptr) {
        logError("Unrecognized simulator ID. Available simulators can be found with the 'list_simulators' command.", "SetSimulatorPauseStateCommand");
        return;
    }
    simulator->setPause((bool)state);
    logInfo("Set pause state of eval {} to: {}", "SetSimulatorPauseStateCommand", simulatorId, simulator->isPause());
}
