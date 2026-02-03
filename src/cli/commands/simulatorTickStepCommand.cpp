#include "simulatorTickStepCommand.h"

#include "backend/evaluator/simulator/evalLogicSimulator.h"
#include "environment/environment.h"
#include "util/runAtStartup.h"
#include "../commandManager.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<SimulatorTickStepCommand>());)

void SimulatorTickStepCommand::run(const std::vector<std::string>& args, Environment& environment) {
    if (args.size() != 2 && args.size() != 3) {
		logError("Wrong number of arguments passed to reset_simulator. Proper usage is 'simulator_tick_step {eval_id} {num_ticks}' or 'simulator_tick_step {eval_id}'", "SimulatorTickStepCommand");
		return;
    }

    int simulatorId;
    int ticks = 1;
    try {
        simulatorId = std::stoi(args[1]);
        if (args.size() == 3) {
            ticks = std::stoi(args[2]);
        }
    }
    catch (...) {
        logError("Exception occured. Check your arguments, they should be reasonably-sized integers.", "SimulatorTickStepCommand");
        return;
    }
    EvalLogicSimulator* simulator = environment.getBackend().getSimulatorManager().getSimulator(simulator_id_t(simulatorId));
    if (simulator == nullptr) {
        logError("Unrecognized simulator ID. Available simulators can be found with the 'list_simulators' command.", "SimulatorTickStepCommand");
        return;
    }
    simulator->tickStep(ticks);
    logInfo("Stepped simulator with ID of {} forward by {} tick(s)", "SimulatorTickStepCommand", simulatorId, ticks);
}
