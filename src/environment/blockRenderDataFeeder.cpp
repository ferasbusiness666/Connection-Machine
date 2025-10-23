#include "blockRenderDataFeeder.h"

#include "computerAPI/directoryManager.h"

#include "backend/backend.h"
#include "gpu/cpuImageEditor/cpuImage.h"
#include "gpu/mainRenderer.h"
#include "gpu/freetype/freetype.h"

BlockRenderDataFeeder::BlockRenderDataFeeder(Backend* backend) : backend(backend), dataUpdateEventReceiver(backend->getDataUpdateEventManager()) {
	dataUpdateEventReceiver.linkFunction("newBlockType", std::bind(&BlockRenderDataFeeder::newBlockTypeUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("postBlockSizeChange", std::bind(&BlockRenderDataFeeder::postBlockSizeChangeUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockNameChange", std::bind(&BlockRenderDataFeeder::blockNameChangeUpdate, this, std::placeholders::_1));

	dataUpdateEventReceiver.linkFunction("blockDataSetConnection", std::bind(&BlockRenderDataFeeder::blockDataSetConnectionUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockDataRemoveConnection", std::bind(&BlockRenderDataFeeder::blockDataRemoveConnectionUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockDataConnectionNameSet", std::bind(&BlockRenderDataFeeder::blockDataConnectionNameSetUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockDataTextureChange", std::bind(&BlockRenderDataFeeder::blockDataTextureChangeUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockDataUsesTileMapTextureChange", std::bind(&BlockRenderDataFeeder::blockDataUsesTileMapTextureChangeUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockDataTextureTileSizeChange", std::bind(&BlockRenderDataFeeder::blockDataTextureTileChangeUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockDataTextureSmallestCordTileChange", std::bind(&BlockRenderDataFeeder::blockDataTextureTileChangeUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockDataTextureBlockTileSizeChange", std::bind(&BlockRenderDataFeeder::blockDataTextureTileChangeUpdate, this, std::placeholders::_1));
	dataUpdateEventReceiver.linkFunction("blockDataTextureBlockStateOffsetChange", std::bind(&BlockRenderDataFeeder::blockDataTextureTileChangeUpdate, this, std::placeholders::_1));

	MainRenderer::get().addBlockTexture((DirectoryManager::getResourceDirectory() / "logicTiles.png").string());
	// MainRenderer::get().addBlockTexture((DirectoryManager::getResourceDirectory() / "gateIcon.png").string());

	CpuImage img({400, 400});

	std::shared_ptr<Font> font = Freetype::get().loadFont(*Settings::get<SettingType::FILE_PATH>("Appearance/Font"), 64);
	logInfo(font->getFontFamily());

	// img.writeString(font, "Berman", {10, 100}, {255, 255, 255, 255}, Rotation::NINETY);
	// img.writeString(font, "Berman", {80, 100}, {255, 255, 255, 255}, Rotation::ZERO);
	// img.writeString(font, "Berman", {320, 300}, {255, 255, 255, 255}, Rotation::ONE_EIGHTY);
	// img.writeString(font, "Berman", {380, 300}, {255, 255, 255, 255}, Rotation::TWO_SEVENTY);
	img.writeStringInArea(font, "I hope this wraps YAY BERMAN BERMAN BERMAN JACK J connection machine", {10, 10}, {80, 200}, {255, 255, 255, 255}, Rotation::NINETY);
	img.writeStringInArea(font, "I hope this wraps YAY BERMAN BERMAN BERMAN JACK J connection machine", {10, 200}, {100, 100}, {255, 255, 255, 255}, Rotation::ONE_EIGHTY);
	img.writeStringInArea(font, "I hope this wraps YAY BERMAN BERMAN BERMAN JACK J connection machine", {10, 10}, {80, 200}, {255, 255, 255, 255}, Rotation::NINETY);
	img.addRect({40, 40}, {320, 320}, {255, 0, 0, 100});

	noTextureTexture = MainRenderer::get().addBlockTexture(img.getData(), img.getSize().x, img.getSize().y);
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
	const BlockData* blockData = backend->getBlockDataManager()->getBlockData(data->get());
	if (!blockData) {
		logError("Failed to find BlockData for BlockType {}", "BlockRenderDataFeeder", data->get());
		return;
	}

	BlockRenderDataId blockRenderDataId = MainRenderer::get().registerBlockRenderData();
	blockTypeToRenderData.emplace(data->get(), blockRenderDataId);

	if (!blockData->getUsesTileMapTexture()) {
		if (blockData->getTexturePath() == "") {
			BlockTextureId blockTextureId = noTextureTexture; //MainRenderer::get().addBlockTexture(DirectoryManager::getResourceDirectory() / "gateIcon.png");
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(blockRenderDataId, blockTextureId);
		} else {
			BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(blockData->getTexturePath());
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(blockRenderDataId, blockTextureId);
		}
	} else {
		if (blockData->getTexturePath() == "") {
			BlockTextureId blockTextureId = noTextureTexture; //MainRenderer::get().addBlockTexture(DirectoryManager::getResourceDirectory() / "gateIcon.png");
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(
				blockRenderDataId,
				blockTextureId,
				blockData->getTextureTileSize(),
				blockData->getTextureSmallestCordTile(),
				blockData->getTextureBlockTileSize(),
				blockData->getTextureBlockStateOffset()
			);
		} else {
			BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(blockData->getTexturePath());
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(
				blockRenderDataId,
				blockTextureId,
				blockData->getTextureTileSize(),
				blockData->getTextureSmallestCordTile(),
				blockData->getTextureBlockTileSize(),
				blockData->getTextureBlockStateOffset()
			);
		}
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
			blockData->getConnectionVector(data->get().second)->free() + (isInput ? blockData->getConnectionPortOffset(data->get().second).value() : blockData->getConnectionPortOffset(data->get().second).value())
		);
		iter->second.blockPortRenderDataIds.try_emplace(data->get().second, blockPortRenderDataId);
		return;
	}
	MainRenderer::get().moveBlockPort(
		iter->second.blockRenderDataId,
		portIter->second,
		blockData->getConnectionVector(data->get().second)->free() + (isInput ? blockData->getConnectionPortOffset(data->get().second).value() : blockData->getConnectionPortOffset(data->get().second).value())
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

	const BlockData* blockData = backend->getBlockDataManager()->getBlockData(data->get().first);
	if (!blockData) {
		logError("Failed to find BlockData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}

	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}

	if (!blockData->getUsesTileMapTexture()) {
		if (blockData->getTexturePath() == "") {
			BlockTextureId blockTextureId = noTextureTexture; //MainRenderer::get().addBlockTexture(DirectoryManager::getResourceDirectory() / "gateIcon.png");
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(iter->second.blockRenderDataId, blockTextureId);
		} else {
			BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(blockData->getTexturePath());
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(iter->second.blockRenderDataId, blockTextureId);
		}
	} else {
		if (blockData->getTexturePath() == "") {
			BlockTextureId blockTextureId =noTextureTexture; //MainRenderer::get().addBlockTexture(DirectoryManager::getResourceDirectory() / "gateIcon.png");
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(
				iter->second.blockRenderDataId,
				blockTextureId,
				blockData->getTextureTileSize(),
				blockData->getTextureSmallestCordTile(),
				blockData->getTextureBlockTileSize(),
				blockData->getTextureBlockStateOffset()
			);
		} else {
			BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(blockData->getTexturePath());
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(
				iter->second.blockRenderDataId,
				blockTextureId,
				blockData->getTextureTileSize(),
				blockData->getTextureSmallestCordTile(),
				blockData->getTextureBlockTileSize(),
				blockData->getTextureBlockStateOffset()
			);
		}
	}
}

void BlockRenderDataFeeder::blockDataUsesTileMapTextureChangeUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, bool>>();
	if (!data) return;

	const BlockData* blockData = backend->getBlockDataManager()->getBlockData(data->get().first);
	if (!blockData) {
		logError("Failed to find BlockData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}

	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {} \"{}\"", "BlockRenderDataFeeder", data->get().first, blockData->getName());
		return;
	}

	if (!blockData->getUsesTileMapTexture()) {
		if (blockData->getTexturePath() == "") {
			BlockTextureId blockTextureId =noTextureTexture; // MainRenderer::get().addBlockTexture(DirectoryManager::getResourceDirectory() / "gateIcon.png");
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(iter->second.blockRenderDataId, blockTextureId);
		} else {
			BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(blockData->getTexturePath());
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(iter->second.blockRenderDataId, blockTextureId);
		}
	} else {
		if (blockData->getTexturePath() == "") {
			BlockTextureId blockTextureId = noTextureTexture; //MainRenderer::get().addBlockTexture(DirectoryManager::getResourceDirectory() / "gateIcon.png");
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(
				iter->second.blockRenderDataId,
				blockTextureId,
				blockData->getTextureTileSize(),
				blockData->getTextureSmallestCordTile(),
				blockData->getTextureBlockTileSize(),
				blockData->getTextureBlockStateOffset()
			);
		} else {
			BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(blockData->getTexturePath());
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(
				iter->second.blockRenderDataId,
				blockTextureId,
				blockData->getTextureTileSize(),
				blockData->getTextureSmallestCordTile(),
				blockData->getTextureBlockTileSize(),
				blockData->getTextureBlockStateOffset()
			);
		}
	}
}

void BlockRenderDataFeeder::blockDataTextureTileChangeUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, Vec2Int>>();
	if (!data) return;

	const BlockData* blockData = backend->getBlockDataManager()->getBlockData(data->get().first);
	if (!blockData) {
		logError("Failed to find BlockData for BlockType {}", "BlockRenderDataFeeder", data->get().first);
		return;
	}
	if (!blockData->getUsesTileMapTexture()) return;

	auto iter = blockTypeToRenderData.find(data->get().first);
	if (iter == blockTypeToRenderData.end()) {
		logError("Failed to find RenderData for BlockType {} \"{}\"", "BlockRenderDataFeeder", data->get().first, blockData->getName());
		return;
	}

	if (blockData->getTexturePath() == "") {
		BlockTextureId blockTextureId = noTextureTexture; //MainRenderer::get().addBlockTexture(DirectoryManager::getResourceDirectory() / "gateIcon.png");
		if (blockTextureId == 0) return;
		MainRenderer::get().setBlockTexture(
			iter->second.blockRenderDataId,
			blockTextureId,
			blockData->getTextureTileSize(),
			blockData->getTextureSmallestCordTile(),
			blockData->getTextureBlockTileSize(),
			blockData->getTextureBlockStateOffset()
		);
	} else {
		BlockTextureId blockTextureId = MainRenderer::get().addBlockTexture(blockData->getTexturePath());
		if (blockTextureId == 0) return;
		MainRenderer::get().setBlockTexture(
			iter->second.blockRenderDataId,
			blockTextureId,
			blockData->getTextureTileSize(),
			blockData->getTextureSmallestCordTile(),
			blockData->getTextureBlockTileSize(),
			blockData->getTextureBlockStateOffset()
		);
	}
}

void BlockRenderDataFeeder::refreshBlockTexture(BlockType blockType) {
	const BlockData* blockData = backend->getBlockDataManager()->getBlockData(blockType);
	if (!blockData) {
		logError("Failed to find BlockData to refresh block texture for BlockType {}", "BlockRenderDataFeeder", blockType);
		return;
	}

	MainRenderer::get().refreshBlockTexture(blockData->getTexturePath());
}