#include "blockData.h"

BlockData::BlockData(BlockType blockType, DataUpdateEventManager* dataUpdateEventManager) : blockType(blockType), dataUpdateEventManager(dataUpdateEventManager) {}

void BlockData::setDefaultData(bool defaultData) noexcept {
	if (defaultData == this->defaultData) return;
	bool sentPre = false;
	if (defaultData) {
		for (auto connection : connections) {
			dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("preBlockDataRemoveConnection", { blockType, connection.first });
			dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("preBlockDataSetConnection", { blockType, connection.first });
		}
		if (getSize() != Size(1)) {
			dataUpdateEventManager->sendEvent<std::pair<BlockType, Size>>("preBlockSizeChange", { blockType, Size(1) });
			sentPre = true;
		}
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("preBlockDataSetConnection", { blockType, 0 });
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("preBlockDataSetConnection", { blockType, 1 });
	}
	this->defaultData = defaultData;
	blockSize = Size(1);

	if (defaultData) {
		if (sentPre) {
			dataUpdateEventManager->sendEvent<std::pair<BlockType, Size>>("postBlockSizeChange", { blockType, Size(1) });
		}
		for (auto connection : connections) {
			dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataRemoveConnection", { blockType, connection.first });
			dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataSetConnection", { blockType, connection.first });
		}
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataSetConnection", { blockType, 0 });
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataSetConnection", { blockType, 1 });
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataConnectionNameSet", { blockType, 0 });
		dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataConnectionNameSet", { blockType, 1 });
		connections.clear();
	}
	sendBlockDataUpdate();
}

void BlockData::setSize(Size size) noexcept {
	if (getSize() == size) return;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, Size>>("preBlockSizeChange", { blockType, size });
	blockSize = size;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, Size>>("postBlockSizeChange", { blockType, getSize() });
	sendBlockDataUpdate();
}

void BlockData::setName(const std::string& name) noexcept {
	this->name = name;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, std::string>>("blockNameChange", { blockType, name });
	sendBlockDataUpdate();
}

void BlockData::setPath(const std::string& path) noexcept {
	this->path = path;
	sendBlockDataUpdate();
}

void BlockData::setTexturePath(const std::string& texturePath) noexcept {
	if (this->texturePath == texturePath) return; // what is this going to do...
	this->texturePath = texturePath;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, std::string>>("blockDataTextureChange", { blockType, texturePath });
	sendBlockDataUpdate();
}

void BlockData::setTextureTileSize(Vec2Int tileSize) noexcept {
	if (this->textureTileSize == tileSize) return;
	this->textureTileSize = tileSize;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, Vec2Int>>("blockDataTextureTileSizeChange", { blockType, tileSize });
	sendBlockDataUpdate();
}

void BlockData::setTextureSmallestCordTile(Vec2Int smallestCordTile) noexcept {
	if (this->textureSmallestCordTile == smallestCordTile) return;
	this->textureSmallestCordTile = smallestCordTile;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, Vec2Int>>("blockDataTextureSmallestCordTileChange", { blockType, smallestCordTile });
	sendBlockDataUpdate();
}

void BlockData::setTextureBlockTileSize(Vec2Int blockSizeInTiles) noexcept {
	if (this->textureBlockTileSize == blockSizeInTiles) return;
	this->textureBlockTileSize = blockSizeInTiles;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, Vec2Int>>("blockDataTextureBlockTileSizeChange", { blockType, blockSizeInTiles });
	sendBlockDataUpdate();
}

void BlockData::setTextureBlockStateOffset(Vec2Int textureBlockStateOffset) noexcept {
	if (this->textureBlockStateOffset == textureBlockStateOffset) return;
	this->textureBlockStateOffset = textureBlockStateOffset;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, Vec2Int>>("blockDataTextureBlockStateOffsetChange", { blockType, textureBlockStateOffset });
	sendBlockDataUpdate();
}

void BlockData::setUsesTileMapTexture(bool usesTileMapTexture) noexcept {
	if (this->usesTileMapTexture == usesTileMapTexture) return;
	this->usesTileMapTexture = usesTileMapTexture;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, bool>>("blockDataUsesTileMapTextureChange", { blockType, usesTileMapTexture });
	sendBlockDataUpdate();
}

// trys to set a connection input in the block. Returns success.
void BlockData::removeConnection(connection_end_id_t connectionId) noexcept {
	auto iter = connections.find(connectionId);
	if (iter == connections.end()) return;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("preBlockDataRemoveConnection", { blockType, connectionId });
	if (iter->second.portType == ConnectionData::PortType::INPUT) {
		inputConnectionCount--;
	} else if (iter->second.portType == ConnectionData::PortType::OUTPUT) {
		outputConnectionCount--;
	}
	connections.erase(iter);
	dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataRemoveConnection", { blockType, connectionId });
	sendBlockDataUpdate();
}
void BlockData::setConnectionInput(Vector positionOnBlock, connection_end_id_t connectionId) noexcept {
	dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("preBlockDataSetConnection", { blockType, connectionId });
	connections.insert_or_assign(connectionId, ConnectionData(positionOnBlock, ConnectionData::PortType::INPUT));
	inputConnectionCount++;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataSetConnection", { blockType, connectionId });
	sendBlockDataUpdate();
}
// trys to set a connection output in the block. Returns success.
void BlockData::setConnectionOutput(Vector positionOnBlock, connection_end_id_t connectionId) noexcept {
	dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("preBlockDataSetConnection", { blockType, connectionId });
	connections.insert_or_assign(connectionId, ConnectionData(positionOnBlock, ConnectionData::PortType::OUTPUT));
	outputConnectionCount++;
	dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataSetConnection", { blockType, connectionId });
	sendBlockDataUpdate();
}

void BlockData::setConnectionBidirectional(Vector positionOnBlock, connection_end_id_t connectionId) noexcept {
	dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("preBlockDataSetConnection", { blockType, connectionId });
	connections.insert_or_assign(connectionId, ConnectionData(positionOnBlock, ConnectionData::PortType::BIDIRECTIONAL));
	dataUpdateEventManager->sendEvent<std::pair<BlockType, connection_end_id_t>>("blockDataSetConnection", { blockType, connectionId });
	sendBlockDataUpdate();
}

void BlockData::setConnectionIdName(connection_end_id_t connectionId, const std::string& name) {
	connectionIdNames.set(connectionId, name);
	dataUpdateEventManager->sendEvent(
		"blockDataConnectionNameSet",
		DataUpdateEventManager::EventDataWithValue<std::pair<BlockType, connection_end_id_t>>({ blockType, connectionId })
	);
}
