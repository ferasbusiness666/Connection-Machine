#include "listEvaluatorsCommand.h"

#include "environment/environment.h"
#include "../commandManager.h"
#include "util/runAtStartup.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<ListEvaluatorsCommand>());)

void ListEvaluatorsCommand::run(const std::vector<std::string>& args, Environment& environment) {
    std::string evaluatorDetails = "";
	for (auto iter = environment.getBackend().getEvaluatorManager().begin(); iter != environment.getBackend().getEvaluatorManager().end(); iter++) {
        evaluatorDetails = evaluatorDetails + iter->second->getEvaluatorName() + "\n";
    }
    std::cout << evaluatorDetails << std::flush;
}