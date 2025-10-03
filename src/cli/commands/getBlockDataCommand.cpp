#include "getBlockDataCommand.h"

#include "environment/environment.h"
#include "../commandManager.h"
#include "util/runAtStartup.h"

runAtStartup(CommandManager::get().registerCommand(std::make_unique<GetBlockDataCommand>());)

void GetBlockDataCommand::run(const std::vector<std::string>& args, Environment& environment) {
    if (args.size() != 4) {
		  logError("Wrong number of arguments passed to get_block_data. Proper usage is 'get_block_data {UUID} {x} {y}'.", "GetBlockDataCommand");
		  return;
    }
    SharedCircuit cir = environment.getBackend().getCircuitManager().getCircuit(args[1]);
    if (cir == nullptr) {
      logError("Unrecognized UUID. Available circuits can be found with the 'list_circuits' command.", "GetBlockDataCommand");
    }
}