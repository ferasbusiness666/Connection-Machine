#include "listCircuitsCommand.h"

#include "environment/environment.h"
#include "../commandManager.h"
#include "util/runAtStartup.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<ListCircuitsCommand>());)

void ListCircuitsCommand::run(const std::vector<std::string>& args, Environment& environment) {
    std::string circuitDetails = "";
	for (auto iter = environment.getBackend().getCircuitManager().begin(); iter != environment.getBackend().getCircuitManager().end(); iter++) {
        circuitDetails = circuitDetails + iter->second->getCircuitName() + " (" + iter->second->getUUID() + ")\n";
    }
    std::cout << circuitDetails << std::flush;
}