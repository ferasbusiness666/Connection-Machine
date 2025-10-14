#include "setEvaluatorStateCommand.h"

#include "environment/environment.h"
#include "../commandManager.h"
#include "util/runAtStartup.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<SetEvaluatorStateCommand>());)

void SetEvaluatorStateCommand::run(const std::vector<std::string>& args, Environment& environment) {
	if (args.size() != 3) {
		logError("Wrong number of arguments passed to set_evaluator_state. Proper usage is 'set_evaluator_state {eval_id} {state}'", "SetEvaluatorStateCommand");
		return;
	}

    int evalID;
    int state;
    try {
        evalID = std::stoi(args[1]);
        state = std::stoi(args[2]);
    }
    catch (...) {
        logError("Exception occured. Check your arguments, they should be reasonably-sized integers.", "SetEvaluatorStateCommand");
        return;
    }
    if (state < 0 || state > 3) {
        logError("Invalid state. Valid states are 0 for low, 1 for high, 2 for floating, and 3 for undefined.");
        return;
    }
    SharedEvaluator eval = environment.getBackend().getEvaluatorManager().getEvaluator(evalID);
    if (eval == nullptr) {
        logError("Unrecognized evaluator ID. Available evaluators can be found with the 'list_evaluators' command.", "SetEvaluatorStateCommand");
        return;
    }
    eval->setState(Address(), (logic_state_t)state);
}
