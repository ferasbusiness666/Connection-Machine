#include "createEvaluatorCommand.h"

#include "environment/environment.h"
#include "../commandManager.h"
#include "util/runAtStartup.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<CreateEvaluatorCommand>());)

void CreateEvaluatorCommand::run(const std::vector<std::string>& args, Environment& environment) {
    if (args.size() != 2) {
  		logError("Wrong number of arguments passed to create_evaluator. Proper usage is 'create_evaluator {UUID}'.", "CreateEvaluatorCommand");
	  	return;
    }
    SharedCircuit cir = environment.getBackend().getCircuitManager().getCircuit(args[1]);
    if (cir == nullptr) {
        logError("Unrecognized UUID. Available circuits can be found with the 'list_circuits' command.", "CreateEvaluatorCommand");
        return;
    }
    circuit_id_t id = cir->getCircuitId();
    auto backend = environment.getBackend();
    backend.createEvaluator(id);
}