#include "listSimulatorsCommand.h"

#include "backend/evaluator/simulator/evalLogicSimulator.h"
#include "environment/environment.h"
#include "util/runAtStartup.h"
#include "../commandManager.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<ListSimulatorsCommand>());)

void ListSimulatorsCommand::run(const std::vector<std::string>& args, Environment& environment) {
//    std::string simulatorDetails = "";
	for (auto iter = environment.getBackend().getSimulatorManager().begin(); iter != environment.getBackend().getSimulatorManager().end(); iter++) {
//        simulatorDetails = simulatorDetails + iter->second->getSimulatorName() + ", Paused: " + + "\n";
        logInfo("{}, Paused: {}", "ListSimulatorsCommand", iter->second->getSimulatorName(), iter->second->isPause());
    }
//    std::cout << simulatorDetails << std::flush;
}