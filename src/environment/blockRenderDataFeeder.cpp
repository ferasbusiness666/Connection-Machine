#include "blockRenderDataFeeder.h"

#include "backend/backend.h"
#include "gpu/freetype/freetype.h"
#include "gpu/mainRenderer.h"

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

	font = Freetype::get().loadFont(*Settings::get<SettingType::FILE_PATH>("Appearance/Font"), 64);
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
	blockTypeToRenderData.emplace(data->get(), blockRenderDataId);

	blockTexturesToUpdate.insert(data->get());
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
	blockTexturesToUpdate.insert(data->get().first);
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
	blockTexturesToUpdate.insert(data->get().first);
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
			blockData->getConnectionVector(data->get().second)->free() +
				(isInput ? blockData->getConnectionPortOffset(data->get().second).value() : blockData->getConnectionPortOffset(data->get().second).value())
		);
		iter->second.blockPortRenderDataIds.try_emplace(data->get().second, blockPortRenderDataId);
		return;
	}
	MainRenderer::get().moveBlockPort(
		iter->second.blockRenderDataId,
		portIter->second,
		blockData->getConnectionVector(data->get().second)->free() +
			(isInput ? blockData->getConnectionPortOffset(data->get().second).value() : blockData->getConnectionPortOffset(data->get().second).value())
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
	blockTexturesToUpdate.insert(data->get().first);
}

void BlockRenderDataFeeder::blockDataUsesTileMapTextureChangeUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, bool>>();
	if (!data) return;
	blockTexturesToUpdate.insert(data->get().first);
}

void BlockRenderDataFeeder::blockDataTextureTileChangeUpdate(const DataUpdateEventManager::EventData* dataEvent) {
	const auto* data = dataEvent->cast<std::pair<BlockType, Vec2Int>>();
	if (!data) return;
	blockTexturesToUpdate.insert(data->get().first);
}

void BlockRenderDataFeeder::refreshBlockTexture(BlockType blockType) {
	blockTexturesToUpdate.insert(blockType);
}

BlockTextureId BlockRenderDataFeeder::getBlockTextureId(const BlockData* blockData, RenderData* renderData) {
	if (!blockData || !renderData) return 0;

	BlockTextureId blockTextureId = 0;
	if (blockData->getTexturePath().empty()) {
		// create new block texture
		int scale = 256;
		int width = blockData->getSize().w * scale;
		int height = blockData->getSize().h * scale;
		auto [xPadding, yPadding] = calculatePadding(width, height);
		int paddedWidth = width + xPadding * 2;
		int paddedHeight = height + yPadding * 2;
		if (paddedWidth > BLOCK_TEXTURE_SIZE || paddedHeight > BLOCK_TEXTURE_SIZE) {
			if (paddedWidth > paddedHeight) {
				scale = (BLOCK_TEXTURE_SIZE - xPadding * 2) / blockData->getSize().w;
			} else {
				scale = (BLOCK_TEXTURE_SIZE - yPadding * 2) / blockData->getSize().h;
			}
		}
		CpuImage img({ blockData->getSize().w * scale, blockData->getSize().h * scale }, { 0, 0, 0, 255 });
		if (blockData->isBus()) {
			createTextureForBusBlock(blockData, img, scale);
		} else {
			createTextureForCustomBlock(blockData, img, scale);
		}
		CpuImage paddedImg = padTexture(img);
		blockTextureId = MainRenderer::get().addBlockTexture(paddedImg.getData(), paddedImg.getSize().x, paddedImg.getSize().y);
	} else {
		blockTextureId = MainRenderer::get().addBlockTexture(blockData->getTexturePath());
	}
	if (renderData->blockTextureId == blockTextureId) return blockTextureId;
	if (renderData->blockTextureId != 0) {
		auto iter = blockTextureIdUsage.find(renderData->blockTextureId);
		if (iter->second <= 1) {
			blockTextureIdUsage.erase(iter);
			MainRenderer::get().removeBlockTexture(renderData->blockTextureId);
		} else {
			iter->second--;
		}
	}
	renderData->blockTextureId = blockTextureId;
	blockTextureIdUsage[blockTextureId]++;
	return blockTextureId;
}

std::pair<int, int> BlockRenderDataFeeder::calculatePadding(int width, int height) {
	float xPadding = 1.0f;
	float heightToWidthRatio = (float)height / (float)width;
	float yPadding = xPadding * heightToWidthRatio;
	if (yPadding < 1.0f) {
		yPadding = 1.0f;
		xPadding = yPadding / heightToWidthRatio;
	}
	int xPixelPadding = static_cast<int>(std::round(xPadding));
	int yPixelPadding = static_cast<int>(std::round(yPadding));
	return { xPixelPadding, yPixelPadding };
}

CpuImage BlockRenderDataFeeder::padTexture(const CpuImage& img) {
	int width = img.getSize().x;
	int height = img.getSize().y;
	auto [xPixelPadding, yPixelPadding] = calculatePadding(width, height);
	Vec2Int newSize = Vec2Int(
		width + 2 * xPixelPadding,
		height + 2 * yPixelPadding
	);
	CpuImage paddedImg(newSize, { 0, 0, 0, 0 });
	for (int y = 0; y < height + yPixelPadding * 2; y++) {
		for (int x = 0; x < width + xPixelPadding * 2; x++) {
			int srcX = std::clamp(x - xPixelPadding, 0, width - 1);
			int srcY = std::clamp(y - yPixelPadding, 0, height - 1);
			paddedImg.setPixel({ x, y }, img.getPixel({ srcX, srcY }));
		}
	}
	return paddedImg;
}

void BlockRenderDataFeeder::putConnectionNipplesOntoImage(const BlockData* blockData, CpuImage& img, int scale) {
	for (const std::pair<connection_end_id_t, BlockData::ConnectionData>& connection : blockData->getConnections()) {
		Vec2Int portTexturePos = (
			Vec2Int(connection.second.positionOnBlock.dx * scale, connection.second.positionOnBlock.dy * scale) +
			Vec2Int(connection.second.portOffset.dx * (float)scale, connection.second.portOffset.dy * (float)scale)
		);
		int laneCount = connection.second.getBitWidth();
		int nippleSize = std::max(9, std::min(9 * laneCount, 9 * 8));
		img.addCircle(portTexturePos, nippleSize * scale / 256, { 0, 0, 0, 255 });
	}
}

void BlockRenderDataFeeder::createTextureForCustomBlock(const BlockData* blockData, CpuImage& img, int scale) {
	img.addRect({ 5 * scale / 256, 5 * scale / 256 }, img.getSize() - Vec2Int(10 * scale / 256, 10 * scale / 256), { 76, 76, 76, 255 });
	putConnectionNipplesOntoImage(blockData, img, scale);
	if (blockData->getSize().w >= blockData->getSize().h) {
		img.writeStringInArea(font, blockData->getName(), { 75*scale/256, 75*scale/256 }, img.getSize() - Vec2Int(150*scale/256, 150*scale/256), { 255, 255, 255, 255 }, Rotation::ZERO, true, true);
	} else {
		img.writeStringInArea(font, blockData->getName(), { 75*scale/256, 75*scale/256 }, img.getSize() - Vec2Int(150*scale/256, 150*scale/256), { 255, 255, 255, 255 }, Rotation::NINETY, true, true);
	}
}

void BlockRenderDataFeeder::createTextureForBusBlock(const BlockData* blockData, CpuImage& img, int scale) {
	img.fill({ 0, 0, 0, 0 });
	int maxRadius = 0;
	int minY;
	int maxY;
	bool first = true;
	for (const std::pair<connection_end_id_t, BlockData::ConnectionData>& connection : blockData->getConnections()) {
		Vec2Int portTexturePos = (
			Vec2Int(connection.second.positionOnBlock.dx * scale, connection.second.positionOnBlock.dy * scale) +
			Vec2Int(connection.second.portOffset.dx * (float)scale, connection.second.portOffset.dy * (float)scale)
		);
		int laneCount = connection.second.getBitWidth();
		int nippleSize = std::max(9, std::min(9 * laneCount, 9 * 8)) * scale / 256;
		if (nippleSize > maxRadius) maxRadius = nippleSize;
		int thisMinY = portTexturePos.y - nippleSize;
		int thisMaxY = portTexturePos.y + nippleSize;
		if (first || thisMinY < minY) minY = thisMinY;
		if (first || thisMaxY > maxY) maxY = thisMaxY;
		first = false;
		// {img.getSize().x / 2, portTexturePos.y}
		img.addCircle(portTexturePos, nippleSize, { 76, 76, 76, 255 });
		int x1 = img.getSize().x / 2;
		int x2 = portTexturePos.x;
		img.addRect(
			{ std::min(x1, x2), portTexturePos.y - nippleSize },
			{ std::abs(x1 - x2), nippleSize * 2 },
			{ 76, 76, 76, 255 }
		);
	}

	// int usingRadius = maxRadius;
	int usingRadius = 9 * 4;

	img.addLine(
		{ img.getSize().x / 2, minY + usingRadius },
		{ img.getSize().x / 2, maxY - usingRadius },
		usingRadius * scale / 256 + 1,
		{ 76, 76, 76, 255 },
		true
	);
}

void BlockRenderDataFeeder::doBlockTextureUpdates() {
	for (BlockType blockType : blockTexturesToUpdate) {
		const BlockData* blockData = backend->getBlockDataManager()->getBlockData(blockType);
		if (!blockData) {
			logError("Failed to find BlockData for BlockType {}", "BlockRenderDataFeeder", blockType);
			return;
		}

		auto iter = blockTypeToRenderData.find(blockType);
		if (iter == blockTypeToRenderData.end()) {
			logError("Failed to find RenderData for BlockType {} \"{}\"", "BlockRenderDataFeeder", blockType, blockData->getName());
			return;
		}

		if (!blockData->getUsesTileMapTexture()) {
			BlockTextureId blockTextureId = getBlockTextureId(blockData, &iter->second);
			if (blockTextureId == 0) return;
			MainRenderer::get().setBlockTexture(iter->second.blockRenderDataId, blockTextureId);
		} else {
			BlockTextureId blockTextureId = getBlockTextureId(blockData, &iter->second);
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
	blockTexturesToUpdate.clear();
}
