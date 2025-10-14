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

    circuit_id_t cirID;
    try {
        cirID = std::stoi(args[1]);
    }
    catch (const std::invalid_argument& e) {
        logError("Non-numerical argument detected.", "CreateEvaluatorCommand");
        return;
    }
    catch (const std::out_of_range& e) {
        logError("Positional value is out of range.", "CreateEvaluatorCommand");
        return;
    }
    catch (...) {
        logError("Unknown exception occured.", "CreateEvaluatorCommand");
        return;
    }

    auto backend = environment.getBackend();
    backend.createEvaluator(cirID);
}