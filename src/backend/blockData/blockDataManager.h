#ifndef blockDataManager_h
#define blockDataManager_h

#include "backend/dataUpdateEventManager.h"
#include "computerAPI/directoryManager.h"
#include "blockData.h"

class BlockDataManager {
public:
	BlockDataManager(DataUpdateEventManager* dataUpdateEventManager) : dataUpdateEventManager(dataUpdateEventManager) { }

	void initializeDefaults() {
		assert(blockData.size() == 0); // should call this before doing anything
		// load default data
		for (unsigned int i = 0; i < 14; i++) addBlock();

		std::string mainTexturePath = (DirectoryManager::getResourceDirectory() / "logicTiles.png").string();

		// AND
		BlockData* andBlockData = getBlockData(BlockType::AND);
		andBlockData->setName("And");
		andBlockData->setTexturePath(mainTexturePath);
		andBlockData->setUsesTileMapTexture(true);
		andBlockData->setTextureTileSize({256, 256});
		andBlockData->setTextureBlockTileSize({1, 1});
		andBlockData->setTextureSmallestCordTile({2, 0});
		// OR
		BlockData* orBlockData = getBlockData(BlockType::OR);
		orBlockData->setName("Or");
		orBlockData->setTexturePath(mainTexturePath);
		orBlockData->setUsesTileMapTexture(true);
		orBlockData->setTextureTileSize({256, 256});
		orBlockData->setTextureBlockTileSize({1, 1});
		orBlockData->setTextureSmallestCordTile({3, 0});
		// XOR
		BlockData* xorBlockData = getBlockData(BlockType::XOR);
		xorBlockData->setName("Xor");
		xorBlockData->setTexturePath(mainTexturePath);
		xorBlockData->setUsesTileMapTexture(true);
		xorBlockData->setTextureTileSize({256, 256});
		xorBlockData->setTextureBlockTileSize({1, 1});
		xorBlockData->setTextureSmallestCordTile({4, 0});
		// NAND
		BlockData* nandBlockData = getBlockData(BlockType::NAND);
		nandBlockData->setName("Nand");
		nandBlockData->setTexturePath(mainTexturePath);
		nandBlockData->setUsesTileMapTexture(true);
		nandBlockData->setTextureTileSize({256, 256});
		nandBlockData->setTextureBlockTileSize({1, 1});
		nandBlockData->setTextureSmallestCordTile({5, 0});
		// NOR
		BlockData* norBlockData = getBlockData(BlockType::NOR);
		norBlockData->setName("Nor");
		norBlockData->setTexturePath(mainTexturePath);
		norBlockData->setUsesTileMapTexture(true);
		norBlockData->setTextureTileSize({256, 256});
		norBlockData->setTextureBlockTileSize({1, 1});
		norBlockData->setTextureSmallestCordTile({6, 0});
		// XNOR
		BlockData* xnorBlockData = getBlockData(BlockType::XNOR);
		xnorBlockData->setName("Xnor");
		xnorBlockData->setTexturePath(mainTexturePath);
		xnorBlockData->setUsesTileMapTexture(true);
		xnorBlockData->setTextureTileSize({256, 256});
		xnorBlockData->setTextureBlockTileSize({1, 1});
		xnorBlockData->setTextureSmallestCordTile({7, 0});
		// JUNCTION
		BlockData* junctionBlockData = getBlockData(BlockType::JUNCTION);
		junctionBlockData->setName("Junction");
		junctionBlockData->setTexturePath(mainTexturePath);
		junctionBlockData->setUsesTileMapTexture(true);
		junctionBlockData->setTextureTileSize({256, 256});
		junctionBlockData->setTextureBlockTileSize({1, 1});
		junctionBlockData->setTextureSmallestCordTile({8, 0});
		// TRISTATE_BUFFER
		BlockData* tristateBufferBlockData = getBlockData(BlockType::TRISTATE_BUFFER);
		tristateBufferBlockData->setName("Tristate Buffer");
		tristateBufferBlockData->setDefaultData(false);
		tristateBufferBlockData->setConnectionInput(Vector(0, 1), 0);
		tristateBufferBlockData->setConnectionInput(Vector(0, 0), 1);
		tristateBufferBlockData->setConnectionOutput(Vector(0, 1), 2);
		tristateBufferBlockData->setSize(Size(1, 2));
		tristateBufferBlockData->setTexturePath(mainTexturePath);
		tristateBufferBlockData->setUsesTileMapTexture(true);
		tristateBufferBlockData->setTextureTileSize({256, 256});
		tristateBufferBlockData->setTextureBlockTileSize({1, 1});
		tristateBufferBlockData->setTextureSmallestCordTile({9, 0});
		// BUTTON
		BlockData* buttonBlockData = getBlockData(BlockType::BUTTON);
		buttonBlockData->setName("Button");
		buttonBlockData->setDefaultData(false);
		buttonBlockData->setConnectionOutput(Vector(0), 0);
		buttonBlockData->setTexturePath(mainTexturePath);
		buttonBlockData->setUsesTileMapTexture(true);
		buttonBlockData->setTextureTileSize({256, 256});
		buttonBlockData->setTextureBlockTileSize({1, 1});
		buttonBlockData->setTextureSmallestCordTile({10, 0});
		// TICK_BUTTON
		BlockData* tickButtonBlockData = getBlockData(BlockType::TICK_BUTTON);
		tickButtonBlockData->setName("Tick Button");
		tickButtonBlockData->setDefaultData(false);
		tickButtonBlockData->setConnectionOutput(Vector(0), 0);
		tickButtonBlockData->setTexturePath(mainTexturePath);
		tickButtonBlockData->setUsesTileMapTexture(true);
		tickButtonBlockData->setTextureTileSize({256, 256});
		tickButtonBlockData->setTextureBlockTileSize({1, 1});
		tickButtonBlockData->setTextureSmallestCordTile({11, 0});
		// SWITCH
		BlockData* switchBlockData = getBlockData(BlockType::SWITCH);
		switchBlockData->setName("Switch");
		switchBlockData->setDefaultData(false);
		switchBlockData->setConnectionOutput(Vector(0), 0);
		switchBlockData->setTexturePath(mainTexturePath);
		switchBlockData->setUsesTileMapTexture(true);
		switchBlockData->setTextureTileSize({256, 256});
		switchBlockData->setTextureBlockTileSize({1, 1});
		switchBlockData->setTextureSmallestCordTile({12, 0});
		// CONSTANT
		BlockData* constantBlockData = getBlockData(BlockType::CONSTANT);
		constantBlockData->setName("Constant");
		constantBlockData->setDefaultData(false);
		constantBlockData->setIsPlaceable(false);
		constantBlockData->setConnectionOutput(Vector(0), 0);
		constantBlockData->setTexturePath(mainTexturePath);
		constantBlockData->setUsesTileMapTexture(true);
		constantBlockData->setTextureTileSize({256, 256});
		constantBlockData->setTextureBlockTileSize({1, 1});
		constantBlockData->setTextureSmallestCordTile({13, 0});
		// LIGHT
		BlockData* lightBlockData = getBlockData(BlockType::LIGHT);
		lightBlockData->setName("Light");
		lightBlockData->setDefaultData(false);
		lightBlockData->setConnectionInput(Vector(0), 0);
		lightBlockData->setTexturePath(mainTexturePath);
		lightBlockData->setUsesTileMapTexture(true);
		lightBlockData->setTextureTileSize({256, 256});
		lightBlockData->setTextureBlockTileSize({1, 1});
		lightBlockData->setTextureSmallestCordTile({14, 0});
		// BUS_INTERFACE
		BlockData* busInterfaceBlockData = getBlockData(BlockType::BUS_INTERFACE);
		busInterfaceBlockData->setName("Bus Interface");
		busInterfaceBlockData->setDefaultData(false);
		busInterfaceBlockData->setIsBus(true);
		busInterfaceBlockData->setSize(Size(2, 8));
		busInterfaceBlockData->setConnectionOutput(Vector(1, 0), 0);
		busInterfaceBlockData->setConnectionBitConfiguration(0, std::vector<unsigned int>{0,1,2,3,4,5,6,7});
		busInterfaceBlockData->setConnectionInput(Vector(1, 0), 1);
		busInterfaceBlockData->setConnectionBitConfiguration(1, std::vector<unsigned int>{0,1,2,3,4,5,6,7});
		for (unsigned int i = 0; i < 8; ++i) {
			busInterfaceBlockData->setConnectionOutput(Vector(0, i), i * 2 + 2);
			busInterfaceBlockData->setConnectionBitConfiguration(i * 2 + 2, std::vector<unsigned int>{ i });
			busInterfaceBlockData->setConnectionInput(Vector(0, i), i * 2 + 3);
			busInterfaceBlockData->setConnectionBitConfiguration(i * 2 + 3, std::vector<unsigned int>{ i });
		}
		busInterfaceBlockData->setTexturePath((DirectoryManager::getResourceDirectory() / "gateIcon.png").string());
	}

	inline BlockType addBlock() noexcept {
		blockData.emplace_back((BlockType)(blockData.size() + 1), dataUpdateEventManager);
		BlockType blockType = (BlockType)blockData.size();
		dataUpdateEventManager->sendEvent<BlockType>("newBlockType", blockType);
		// sending data events for default data
		// pre
		dataUpdateEventManager->sendEvent<std::pair<BlockType, Size>>("preBlockSizeChange", { blockType, Size(1) });
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("preBlockDataSetConnection", { blockType, 0 });
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("preBlockDataSetConnection", { blockType, 1 });
		// post
		dataUpdateEventManager->sendEvent<std::pair<BlockType, Size>>("postBlockSizeChange", { blockType, Size(1) });
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataSetConnection", { blockType, 0 });
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataSetConnection", { blockType, 1 });
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataConnectionNameSet", { blockType, 0 });
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataConnectionNameSet", { blockType, 1 });
		sendBlockDataUpdate();
		return blockType;
	}

	inline BlockType getBlockType(const std::string& blockPath) const {
		for (unsigned int i = 0; i < blockData.size(); i++) {
			if (blockData[i].getPath() + "/" + blockData[i].getName() == blockPath) {
				return (BlockType)(i + 1);
			}
		}
		return BlockType::NONE;
	}

	inline void sendBlockDataUpdate() { dataUpdateEventManager->sendEvent("blockDataUpdate"); }

	inline const BlockData* getBlockData(BlockType type) const noexcept { if (!blockExists(type)) return nullptr; return &blockData[type - 1]; }
	inline BlockData* getBlockData(BlockType type) noexcept { if (!blockExists(type)) return nullptr; return &blockData[type - 1]; }

	inline unsigned int maxBlockId() const noexcept { return blockData.size(); }
	inline bool blockExists(BlockType type) const noexcept { return type != BlockType::NONE && type <= blockData.size(); }
	inline bool isPlaceable(BlockType type) const noexcept {
		if (!blockExists(type)) return false;
		return blockData[type - 1].isPlaceable();
	}

	inline std::string getName(BlockType type) const noexcept {
		if (!blockExists(type)) return "None" + std::to_string(type);
		return blockData[type - 1].getName();
	}
	inline std::string getPath(BlockType type) const noexcept {
		if (!blockExists(type)) return "Path To None";
		return blockData[type - 1].getPath();
	}

	inline Size getBlockSize(BlockType type) const noexcept {
		if (!blockExists(type)) return Size();
		return blockData[type - 1].getSize();
	}
	inline Size getBlockSize(BlockType type, Orientation orientation) const noexcept {
		if (!blockExists(type)) return Size();
		return blockData[type - 1].getSize(orientation);
	}

	inline std::optional<connection_end_id_t> getInputConnectionId(BlockType type, Vector vector) const noexcept {
		if (!blockExists(type)) return std::nullopt;
		return blockData[type - 1].getInputConnectionId(vector);
	}
	inline std::optional<connection_end_id_t> getOutputConnectionId(BlockType type, Vector vector) const noexcept {
		if (!blockExists(type)) return std::nullopt;
		return blockData[type - 1].getOutputConnectionId(vector);
	}
	inline  std::optional<connection_end_id_t> getInputConnectionId(BlockType type, Orientation orientation, Vector vector) const noexcept {
		if (!blockExists(type)) return std::nullopt;
		return blockData[type - 1].getInputConnectionId(vector, orientation);
	}
	inline  std::optional<connection_end_id_t> getOutputConnectionId(BlockType type, Orientation orientation, Vector vector) const noexcept {
		if (!blockExists(type)) return std::nullopt;
		return blockData[type - 1].getOutputConnectionId(vector, orientation);
	}
	inline std::optional<Vector> getConnectionVector(BlockType type, connection_end_id_t connectionId) const noexcept {
		if (!blockExists(type)) return std::nullopt;
		return blockData[type - 1].getConnectionVector(connectionId);
	}
	inline std::optional<Vector> getConnectionVector(BlockType type, Orientation orientation, connection_end_id_t connectionId) const noexcept {
		if (!blockExists(type)) return std::nullopt;
		return blockData[type - 1].getConnectionVector(connectionId, orientation);
	}

	inline connection_end_id_t getConnectionCount(BlockType type) const noexcept {
		if (!blockExists(type)) return 0;
		return blockData[type - 1].getConnectionCount();
	}
	inline bool connectionExists(BlockType type, connection_end_id_t connectionId) const noexcept {
		if (!blockExists(type)) return false;
		return blockData[type - 1].connectionExists(connectionId);
	}
	inline bool isConnectionInput(BlockType type, connection_end_id_t connectionId) const noexcept {
		if (!blockExists(type)) return false;
		return blockData[type - 1].isConnectionInput(connectionId);
	}
	inline bool isConnectionOutput(BlockType type, connection_end_id_t connectionId) const noexcept {
		if (!blockExists(type)) return false;
		return blockData[type - 1].isConnectionOutput(connectionId);
	}

private:
	std::vector<BlockData> blockData;
	DataUpdateEventManager* dataUpdateEventManager;
};

#endif /* blockDataManager_h */

