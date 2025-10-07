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
        return;
    }
    int xPos;
    int yPos;
    try {
        xPos = std::stoi(args[2]);
        yPos = std::stoi(args[3]);
    }
    catch (const std::invalid_argument& e) {
        logError("Non-numerical integer value detected.", "GetBlockDataCommand");
        return;
    }
    catch (const std::out_of_range& e) {
        logError("Positional value is out of range.", "GetBlockDataCommand");
        return;
    }
    catch (...) {
        logError("Unknown exception occured.", "GetBlockDataCommand");
        return;
    }
        std::string UUID = args[1];
        const BlockContainer* blockContainer = cir->getBlockContainer();
        Position pos = Position(xPos, yPos);
        const Block* block = blockContainer->getBlock(pos);
        if (block == nullptr) {
            logInfo("No block at position {}.", "GetBlockDataCommand", pos);
        }

        const BlockDataManager* blockDataManager = environment.getBackend().getBlockDataManager();
        const BlockData* blockData = blockDataManager->getBlockData(block->type());
        logInfo("Info for block at position {}", "GetBlockDataCommand", pos);
        logInfo("Type: {}", "GetBlockDataCommand", blockDataManager->getName(block->type()));
        logInfo("Connections:", "GetBlockDataCommand");
        const auto connections = block->getConnectionContainer().getConnections();
        for (auto iter = connections.begin(); iter != connections.end(); iter++) {
            //std::string connIdName = ;
            logInfo("{} (ID: {})", "GetBlockDataCommand", blockData->getConnectionIdToName(iter->first).value(), iter->first);
        }

}