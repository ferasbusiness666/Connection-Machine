#include "getBlockStateCommand.h"

#include "backend/evaluator/evaluator.h"
#include "environment/environment.h"
#include "util/runAtStartup.h"
#include "../commandManager.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<GetBlockStateCommand>());)

void GetBlockStateCommand::run(const std::vector<std::string>& args, Environment& environment) {
	if (args.size() != 4) {
		logError("Wrong number of arguments passed to get_block_state. Proper usage is 'get_block_state {sim_id} {x_pos} {y_pos}'", "GetBlockStateCommand");
		return;
	}

    int simulatorId;
    int xPos;
    int yPos;
    try {
        simulatorId = std::stoi(args[1]);
        xPos = std::stoi(args[2]);
        yPos = std::stoi(args[3]);
    }
    catch (...) {
        logError("Exception occured. Check your arguments, they should be reasonably-sized integers.", "GetBlockStateCommand");
        return;
    }
    const EvalLogicSimulator* simulator = environment.getBackend().getSimulatorManager().getSimulator(simulator_id_t(simulatorId));
    if (simulator == nullptr) {
        logError("Unrecognized simulator ID. Available simulators can be found with the 'list_simulators' command.", "GetBlockStateCommand");
        return;
    }
    unsigned char state = (unsigned char)simulator->getState(Address(Position(xPos, yPos)));
    logInfo("State of block in simulator with ID {} at position ({}, {}) is {}", "GetBlockStateCommand", simulatorId, xPos, yPos, state);
}
