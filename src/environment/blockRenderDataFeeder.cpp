#include "blockRenderDataFeeder.h"

#include "gui/viewportManager/circuitView/renderer/logicRenderingUtils.h"
#include "computerAPI/directoryManager.h"

#include "backend/backend.h"
#include "gpu/mainRenderer.h"

BlockRenderDataFeeder::BlockRenderDataFeeder(Backend* backend) : backend(backend), dataUpdateEventReceiver(backend->getDataUpdateEventManager()) {
	dataUpdateEventReceiver.linkFunction("newBlockType", std::bind(&BlockRenderDataFeeder::newBlockTypeUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("postBlockSizeChange", std::bind(&BlockRenderDataFeeder::postBlockSizeChangeUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockNameChange", std::bind(&BlockRenderDataFeeder::blockNameChangeUpdate, this, std::placeholders::_1));

	dataUpdateEventReceiver.linkFunction("blockDataSetConnection", std::bind(&BlockRenderDataFeeder::blockDataSetConnectionUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockDataRemoveConnection", std::bind(&BlockRenderDataFeeder::blockDataRemoveConnectionUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockDataConnectionNameSet", std::bind(&BlockRenderDataFeeder::blockDataConnectionNameSetUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockDataTextureChange", std::bind(&BlockRenderDataFeeder::blockDataTextureChangeUpdate, this, std::placeholders::_1));

	mainBlockTextureId = MainRenderer::get().addBlockTexture((DirectoryManager::getResourceDirectory() / "logicTiles.png").string());
	otherBlockTextureId = MainRenderer::get().addBlockTexture((DirectoryManager::getResourceDirectory() / "gateIcon.png").string());
}

BlockRenderDataId BlockRenderDataFeeder::getBlockRenderDataId(BlockType blockType) const {
	auto iter = blockTypeToRenderData.find(blockType);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to get BlockRenderDataId for BlockType {}", "BlockRenderDataFeeder", blockType);
		return 0;
	}
	return iter->second.blockRenderDataId;
}

void BlockRenderDataFeeder::newBlockTypeUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<BlockType>();
	if (!data) return;
	BlockRenderDataId blockRenderDataId = MainRenderer::get().registerBlockRenderData();
	auto iter = blockTypeToRenderData.emplace(data->get(), blockRenderDataId);
	RenderData& renderData = iter.first->second;
	if (data->get() == BlockType::BUS_INTERFACE) {
		renderData.texturePath = DirectoryManager::getResourceDirectory() / "gateIcon.png";
		renderData.tileSize = {512, 512};
		renderData.smallestCordTile = {0, 0};
		renderData.blockTileSize = {1, 1};
		MainRenderer::get().setBlockTexture(
			blockRenderDataId,
			otherBlockTextureId,
			renderData.tileSize,
			renderData.smallestCordTile,
			renderData.blockTileSize
		);
	} else if (data->get() < BlockType::CUSTOM) {
		renderData.texturePath = "";
		renderData.tileSize = {256, 256};
		renderData.smallestCordTile = {data->get()+1, 0};
		renderData.blockTileSize = {1, 1};
		MainRenderer::get().setBlockTexture(
			blockRenderDataId,
			mainBlockTextureId,
			renderData.tileSize,
			renderData.smallestCordTile,
			renderData.blockTileSize
		);
	} else {
		renderData.texturePath = "";
		renderData.tileSize = {256, 256};
		renderData.smallestCordTile = {1, 0};
		renderData.blockTileSize = {1, 1};
		MainRenderer::get().setBlockTexture(
			blockRenderDataId,
			mainBlockTextureId,
			renderData.tileSize,
			renderData.smallestCordTile,
			renderData.blockTileSize
		);
	}
}

void BlockRenderDataFeeder::postBlockSizeChangeUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, Size>>();
	if (!data) return;
	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}
	MainRenderer::get().setBlockSize(iter->second.blockRenderDataId, data->get().second);
}

void BlockRenderDataFeeder::blockNameChangeUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, std::string>>();
	if (!data) return;
	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}
	MainRenderer::get().setBlockName(iter->second.blockRenderDataId, data->get().second);
}

void BlockRenderDataFeeder::blockDataSetConnectionUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, connection_end_id_t>>();
	if (!data) return;
	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}
	auto portIter = iter->second.blockPortRenderDataIds.find(data->get().second);
	const BlockData* blockData = backend->getBlockDataManager()->getBlockData(data->get().first);
	bool isInput = blockData->isConnectionInput(data->get().second);
	if (portIter == iter->second.blockPortRenderDataIds.end()) {
		BlockPortRenderDataId blockPortRenderDataId = MainRenderer::get().addBlockPort(
			iter->second.blockRenderDataId,
			isInput,
			blockData->getConnectionVector(data->get().second)->free() + (isInput ? getInputOffset(data->get().first, data->get().second) : getOutputOffset(data->get().first, data->get().second))
		);
		iter->second.blockPortRenderDataIds.try_emplace(data->get().second, blockPortRenderDataId);
		return;
	}
	MainRenderer::get().moveBlockPort(
		iter->second.blockRenderDataId,
		portIter->second,
		blockData->getConnectionVector(data->get().second)->free() + (isInput ? getInputOffset(data->get().first, data->get().second) : getOutputOffset(data->get().first, data->get().second))
	);
}

void BlockRenderDataFeeder::blockDataRemoveConnectionUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, connection_end_id_t>>();
	if (!data) return;
	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}
	auto portIter = iter->second.blockPortRenderDataIds.find(data->get().second);
	if (portIter == iter->second.blockPortRenderDataIds.end()) {
		logError("Failed to remove BlockPortRenderData for BlockType {}, connection_end_id {}", "BlockRenderDataFeeder", data->get().first, data->get().second);
		return;
	}
	MainRenderer::get().removeBlockPort(iter->second.blockRenderDataId, portIter->second);
}

void BlockRenderDataFeeder::blockDataConnectionNameSetUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, connection_end_id_t>>();
	if (!data) return;
	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}
	auto portIter = iter->second.blockPortRenderDataIds.find(data->get().second);
	if (portIter == iter->second.blockPortRenderDataIds.end()) {
		logError("Failed to find BlockPortRenderDataId for BlockType {}, connection_end_id {}", "BlockRenderDataFeeder", data->get().first, data->get().second);
		return;
	}
	const BlockData* blockData = backend->getBlockDataManager()->getBlockData(data->get().first);
	MainRenderer::get().setBlockPortName(iter->second.blockRenderDataId, portIter->second, *blockData->getConnectionIdToName(data->get().second));
}

void BlockRenderDataFeeder::blockDataTextureChangeUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, std::string>>();
	if (!data) return;
	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}

	if (data->get().second == "") {
		iter->second.texturePath = "";
		iter->second.tileSize = {256, 256};
		iter->second.smallestCordTile = {1, 0};
		iter->second.blockTileSize = {1, 1};
		MainRenderer::get().setBlockTexture(
			iter->second.blockRenderDataId,
			mainBlockTextureId,
			iter->second.tileSize,
			iter->second.smallestCordTile,
			iter->second.blockTileSize
		);
		return;
	}

	BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(data->get().second);
	if (blockTextureId == 0) return; // error message already sent

	iter->second.texturePath = data->get().second;
	iter->second.tileSize = {0, 0}; // mean that the whole texture is 1 tile.
	iter->second.smallestCordTile = {0, 0};
	iter->second.blockTileSize = {1, 1};
	MainRenderer::get().setBlockTexture(iter->second.blockRenderDataId, blockTextureId);
}

void BlockRenderDataFeeder::blockDataTextureTileSizeChangeUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, Vec2Int>>();
	if (!data) return;
	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}
	if (iter->second.tileSize == data->get().second) return;
	iter->second.tileSize = data->get().second;
	if (iter->second.tileSize.x == 0 && iter->second.tileSize.y == 0) {
		if (iter->second.texturePath == "") {
			MainRenderer::get().setBlockTexture(iter->second.blockRenderDataId, mainBlockTextureId);
		} else {
			BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(iter->second.texturePath);
			if (blockTextureId == 0) return; // error message already sent
			MainRenderer::get().setBlockTexture(iter->second.blockRenderDataId, blockTextureId);
		}
	} else {
		if (iter->second.texturePath == "") {
			MainRenderer::get().setBlockTexture(
				iter->second.blockRenderDataId,
				mainBlockTextureId,
				iter->second.tileSize,
				iter->second.smallestCordTile,
				iter->second.blockTileSize
			);
		} else {
			BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(iter->second.texturePath);
			if (blockTextureId == 0) return; // error message already sent
			MainRenderer::get().setBlockTexture(
				iter->second.blockRenderDataId,
				blockTextureId,
				iter->second.tileSize,
				iter->second.smallestCordTile,
				iter->second.blockTileSize
			);
		}
	}
}

void BlockRenderDataFeeder::blockDataTextureSmallestCordTileChangeUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, Vec2Int>>();
	if (!data) return;
	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}
	if (iter->second.smallestCordTile == data->get().second) return;
	iter->second.smallestCordTile = data->get().second;
	if (iter->second.tileSize.x == 0 && iter->second.tileSize.y == 0) return;
	if (iter->second.texturePath == "") {
		MainRenderer::get().setBlockTexture(
			iter->second.blockRenderDataId,
			mainBlockTextureId,
			iter->second.tileSize,
			iter->second.smallestCordTile,
			iter->second.blockTileSize
		);
	} else {
		BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(iter->second.texturePath);
		if (blockTextureId == 0) return; // error message already sent
		MainRenderer::get().setBlockTexture(
			iter->second.blockRenderDataId,
			blockTextureId,
			iter->second.tileSize,
			iter->second.smallestCordTile,
			iter->second.blockTileSize
		);
	}
}

void BlockRenderDataFeeder::blockDataTextureBlockTileSizeChangeUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, Vec2Int>>();
	if (!data) return;
	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}
	if (iter->second.blockTileSize == data->get().second) return;
	iter->second.blockTileSize = data->get().second;
	if (iter->second.tileSize.x == 0 && iter->second.tileSize.y == 0) return;
	if (iter->second.texturePath == "") {
		MainRenderer::get().setBlockTexture(
			iter->second.blockRenderDataId,
			mainBlockTextureId,
			iter->second.tileSize,
			iter->second.smallestCordTile,
			iter->second.blockTileSize
		);
	} else {
		BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(iter->second.texturePath);
		if (blockTextureId == 0) return; // error message already sent
		MainRenderer::get().setBlockTexture(
			iter->second.blockRenderDataId,
			blockTextureId,
			iter->second.tileSize,
			iter->second.smallestCordTile,
			iter->second.blockTileSize
		);
	}
}

void BlockRenderDataFeeder::refreshBlockTexture(BlockType blockType) {
	auto iter = blockTypeToRenderData.find(blockType);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to refresh block texture for BlockType {}", "BlockRenderDataFeeder", blockType);
		return;
	}

	MainRenderer::get().refreshBlockTexture(iter->second.texturePath);
}