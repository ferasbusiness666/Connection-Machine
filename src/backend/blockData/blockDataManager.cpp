#include "blockDataManager.h"

#include "computerAPI/directoryManager.h"

void BlockDataManager::initializeDefaults() {
	assert(blockData.size() == 0); // should call this before doing anything
	logInfo("Initializing default BlockData", "BlockDataManager");
	// load default data
	for (unsigned int i = 0; i < 22; i++) addBlock();

	std::string mainTexturePath = (DirectoryManager::getResourceDirectory() / "logicTiles.png").string();

	// AND
	BlockData* andBlockData = getBlockData(BlockType::AND);
	andBlockData->setName("And");
	andBlockData->setConnectionInput(Vector(0), 0);
	andBlockData->setConnectionPortOffset(0, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	andBlockData->setConnectionOutput(Vector(0), 1);
	andBlockData->setConnectionPortOffset(1, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	andBlockData->setVirtualConnection(0, 1);
	andBlockData->newRenderData<BlockData::BlockTextureData>();
	assert(andBlockData->getRenderDataSize() == 1);							  // just to make sure at least once
	assert(andBlockData->isRenderDataOfType<BlockData::BlockTextureData>(0)); // just to make sure at least once
	andBlockData->setBlockTexturePath(0, mainTexturePath);
	andBlockData->setBlockTextureTopLeft(0, { 256, 0 });
	andBlockData->setBlockTextureSize(0, { 256, 256 });
	andBlockData->setBlockTextureVirtualConnection(0, 0);
	andBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// OR
	BlockData* orBlockData = getBlockData(BlockType::OR);
	orBlockData->setName("Or");
	orBlockData->setConnectionInput(Vector(0), 0);
	orBlockData->setConnectionPortOffset(0, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	orBlockData->setConnectionOutput(Vector(0), 1);
	orBlockData->setConnectionPortOffset(1, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	orBlockData->setVirtualConnection(0, 1);
	orBlockData->newRenderData<BlockData::BlockTextureData>();
	orBlockData->setBlockTexturePath(0, mainTexturePath);
	orBlockData->setBlockTextureTopLeft(0, { 2 * 256, 0 });
	orBlockData->setBlockTextureSize(0, { 256, 256 });
	orBlockData->setBlockTextureVirtualConnection(0, 0);
	orBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// XOR
	BlockData* xorBlockData = getBlockData(BlockType::XOR);
	xorBlockData->setName("Xor");
	xorBlockData->setConnectionInput(Vector(0), 0);
	xorBlockData->setConnectionPortOffset(0, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	xorBlockData->setConnectionOutput(Vector(0), 1);
	xorBlockData->setConnectionPortOffset(1, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	xorBlockData->setVirtualConnection(0, 1);
	xorBlockData->newRenderData<BlockData::BlockTextureData>();
	xorBlockData->setBlockTexturePath(0, mainTexturePath);
	xorBlockData->setBlockTextureTopLeft(0, { 3 * 256, 0 });
	xorBlockData->setBlockTextureSize(0, { 256, 256 });
	xorBlockData->setBlockTextureVirtualConnection(0, 0);
	xorBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// NAND
	BlockData* nandBlockData = getBlockData(BlockType::NAND);
	nandBlockData->setName("Nand");
	nandBlockData->setConnectionInput(Vector(0), 0);
	nandBlockData->setConnectionPortOffset(0, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	nandBlockData->setConnectionOutput(Vector(0), 1);
	nandBlockData->setConnectionPortOffset(1, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	nandBlockData->setVirtualConnection(0, 1);
	nandBlockData->newRenderData<BlockData::BlockTextureData>();
	nandBlockData->setBlockTexturePath(0, mainTexturePath);
	nandBlockData->setBlockTextureTopLeft(0, { 4 * 256, 0 });
	nandBlockData->setBlockTextureSize(0, { 256, 256 });
	nandBlockData->setBlockTextureVirtualConnection(0, 0);
	nandBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// NOR
	BlockData* norBlockData = getBlockData(BlockType::NOR);
	norBlockData->setName("Nor");
	norBlockData->setConnectionInput(Vector(0), 0);
	norBlockData->setConnectionPortOffset(0, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	norBlockData->setConnectionOutput(Vector(0), 1);
	norBlockData->setConnectionPortOffset(1, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	norBlockData->setVirtualConnection(0, 1);
	norBlockData->newRenderData<BlockData::BlockTextureData>();
	norBlockData->setBlockTexturePath(0, mainTexturePath);
	norBlockData->setBlockTextureTopLeft(0, { 5 * 256, 0 });
	norBlockData->setBlockTextureSize(0, { 256, 256 });
	norBlockData->setBlockTextureVirtualConnection(0, 0);
	norBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// XNOR
	BlockData* xnorBlockData = getBlockData(BlockType::XNOR);
	xnorBlockData->setName("Xnor");
	xnorBlockData->setConnectionInput(Vector(0), 0);
	xnorBlockData->setConnectionPortOffset(0, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	xnorBlockData->setConnectionOutput(Vector(0), 1);
	xnorBlockData->setConnectionPortOffset(1, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	xnorBlockData->setVirtualConnection(0, 1);
	xnorBlockData->newRenderData<BlockData::BlockTextureData>();
	xnorBlockData->setBlockTexturePath(0, mainTexturePath);
	xnorBlockData->setBlockTextureTopLeft(0, { 6 * 256, 0 });
	xnorBlockData->setBlockTextureSize(0, { 256, 256 });
	xnorBlockData->setBlockTextureVirtualConnection(0, 0);
	xnorBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// BUFFER
	BlockData* bufferBlockData = getBlockData(BlockType::BUFFER);
	bufferBlockData->setName("Buffer");
	bufferBlockData->setConnectionInput(Vector(0), 0);
	bufferBlockData->setConnectionPortOffset(0, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	bufferBlockData->setConnectionOutput(Vector(0), 1);
	bufferBlockData->setConnectionPortOffset(1, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	bufferBlockData->setVirtualConnection(0, 1);
	bufferBlockData->newRenderData<BlockData::BlockTextureData>();
	bufferBlockData->setBlockTexturePath(0, mainTexturePath);
	bufferBlockData->setBlockTextureTopLeft(0, { 7 * 256, 0 });
	bufferBlockData->setBlockTextureSize(0, { 256, 256 });
	bufferBlockData->setBlockTextureVirtualConnection(0, 0);
	bufferBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// NOT
	BlockData* notBlockData = getBlockData(BlockType::NOT);
	notBlockData->setName("Not");
	notBlockData->setConnectionInput(Vector(0), 0);
	notBlockData->setConnectionPortOffset(0, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	notBlockData->setConnectionOutput(Vector(0), 1);
	notBlockData->setConnectionPortOffset(1, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	notBlockData->setVirtualConnection(0, 1);
	notBlockData->newRenderData<BlockData::BlockTextureData>();
	notBlockData->setBlockTexturePath(0, mainTexturePath);
	notBlockData->setBlockTextureTopLeft(0, { 8 * 256, 0 });
	notBlockData->setBlockTextureSize(0, { 256, 256 });
	notBlockData->setBlockTextureVirtualConnection(0, 0);
	notBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// BUTTON
	BlockData* buttonBlockData = getBlockData(BlockType::BUTTON);
	buttonBlockData->setName("Button");
	buttonBlockData->setConnectionOutput(Vector(0), 0);
	buttonBlockData->setConnectionPortOffset(0, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	buttonBlockData->setVirtualConnection(0, 1);
	buttonBlockData->newRenderData<BlockData::BlockTextureData>();
	buttonBlockData->setBlockTexturePath(0, mainTexturePath);
	buttonBlockData->setBlockTextureTopLeft(0, { 9 * 256, 0 });
	buttonBlockData->setBlockTextureSize(0, { 256, 256 });
	buttonBlockData->setBlockTextureVirtualConnection(0, 0);
	buttonBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// TICK_BUTTON
	BlockData* tickButtonBlockData = getBlockData(BlockType::TICK_BUTTON);
	tickButtonBlockData->setName("Tick Button");
	tickButtonBlockData->setConnectionOutput(Vector(0), 0);
	tickButtonBlockData->setConnectionPortOffset(0, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	tickButtonBlockData->setVirtualConnection(0, 1);
	tickButtonBlockData->newRenderData<BlockData::BlockTextureData>();
	tickButtonBlockData->setBlockTexturePath(0, mainTexturePath);
	tickButtonBlockData->setBlockTextureTopLeft(0, { 10 * 256, 0 });
	tickButtonBlockData->setBlockTextureSize(0, { 256, 256 });
	tickButtonBlockData->setBlockTextureVirtualConnection(0, 0);
	tickButtonBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// SWITCH
	BlockData* switchBlockData = getBlockData(BlockType::SWITCH);
	switchBlockData->setName("Switch");
	switchBlockData->setConnectionOutput(Vector(0), 0);
	switchBlockData->setConnectionPortOffset(0, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	switchBlockData->setVirtualConnection(0, 1);
	switchBlockData->newRenderData<BlockData::BlockTextureData>();
	switchBlockData->setBlockTexturePath(0, mainTexturePath);
	switchBlockData->setBlockTextureTopLeft(0, { 11 * 256, 0 });
	switchBlockData->setBlockTextureSize(0, { 256, 256 });
	switchBlockData->setBlockTextureVirtualConnection(0, 0);
	switchBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// CONSTANT OFF
	BlockData* constantOffBlockData = getBlockData(BlockType::CONSTANT_OFF);
	constantOffBlockData->setName("Constant");
	constantOffBlockData->setConnectionOutput(Vector(0), 0);
	constantOffBlockData->setConnectionPortOffset(0, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	constantOffBlockData->setVirtualConnection(0, 1);
	constantOffBlockData->newRenderData<BlockData::BlockTextureData>();
	constantOffBlockData->setBlockTexturePath(0, mainTexturePath);
	constantOffBlockData->setBlockTextureTopLeft(0, { 12 * 256, 0 });
	constantOffBlockData->setBlockTextureSize(0, { 256, 256 });
	constantOffBlockData->setBlockTextureVirtualConnection(0, 0);
	constantOffBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// CONSTANT ON
	BlockData* constantOnBlockData = getBlockData(BlockType::CONSTANT_ON);
	constantOnBlockData->setName("Constant");
	constantOnBlockData->setIsPlaceable(false);
	constantOnBlockData->setConnectionOutput(Vector(0), 0);
	constantOnBlockData->setConnectionPortOffset(0, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	constantOnBlockData->setVirtualConnection(0, 1);
	constantOnBlockData->newRenderData<BlockData::BlockTextureData>();
	constantOnBlockData->setBlockTexturePath(0, mainTexturePath);
	constantOnBlockData->setBlockTextureTopLeft(0, { 12 * 256, 0 });
	constantOnBlockData->setBlockTextureSize(0, { 256, 256 });
	constantOnBlockData->setBlockTextureVirtualConnection(0, 0);
	constantOnBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// CONSTANT Z
	BlockData* constantZBlockData = getBlockData(BlockType::CONSTANT_Z);
	constantZBlockData->setName("Constant");
	constantZBlockData->setIsPlaceable(false);
	constantZBlockData->setConnectionOutput(Vector(0), 0);
	constantZBlockData->setConnectionPortOffset(0, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	constantZBlockData->setVirtualConnection(0, 1);
	constantZBlockData->newRenderData<BlockData::BlockTextureData>();
	constantZBlockData->setBlockTexturePath(0, mainTexturePath);
	constantZBlockData->setBlockTextureTopLeft(0, { 12 * 256, 0 });
	constantZBlockData->setBlockTextureSize(0, { 256, 256 });
	constantZBlockData->setBlockTextureVirtualConnection(0, 0);
	constantZBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// CONSTANT X
	BlockData* constantXBlockData = getBlockData(BlockType::CONSTANT_X);
	constantXBlockData->setName("Constant");
	constantXBlockData->setIsPlaceable(false);
	constantXBlockData->setConnectionOutput(Vector(0), 0);
	constantXBlockData->setConnectionPortOffset(0, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	constantXBlockData->setVirtualConnection(0, 1);
	constantXBlockData->newRenderData<BlockData::BlockTextureData>();
	constantXBlockData->setBlockTexturePath(0, mainTexturePath);
	constantXBlockData->setBlockTextureTopLeft(0, { 12 * 256, 0 });
	constantXBlockData->setBlockTextureSize(0, { 256, 256 });
	constantXBlockData->setBlockTextureVirtualConnection(0, 0);
	constantXBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// LIGHT
	BlockData* lightBlockData = getBlockData(BlockType::LIGHT);
	lightBlockData->setName("Light");
	lightBlockData->setConnectionInput(Vector(0), 0);
	lightBlockData->setConnectionPortOffset(0, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	lightBlockData->setVirtualConnection(0, 1);
	lightBlockData->newRenderData<BlockData::BlockTextureData>();
	lightBlockData->setBlockTexturePath(0, mainTexturePath);
	lightBlockData->setBlockTextureTopLeft(0, { 13 * 256, 0 });
	lightBlockData->setBlockTextureSize(0, { 256, 256 });
	lightBlockData->setBlockTextureVirtualConnection(0, 0);
	lightBlockData->setBlockTextureStateOffset(0, { 0, 256 });
	// JUNCTION
	BlockData* junctionBlockData = getBlockData(BlockType::JUNCTION);
	junctionBlockData->setName("Junction");
	junctionBlockData->setConnectionBidirectional(Vector(0), 0);
	junctionBlockData->setVirtualConnection(0, 1);
	junctionBlockData->newRenderData<BlockData::BlockTextureData>();
	junctionBlockData->setBlockTexturePath(0, mainTexturePath);
	junctionBlockData->setBlockTextureTopLeft(0, { 8 * 256, 6 * 256 });
	junctionBlockData->setBlockTextureSize(0, { 256, 256 });
	junctionBlockData->setBlockTextureVirtualConnection(0, 0);
	junctionBlockData->setBlockTextureStateOffset(0, { 256, 0 });

	// JUNCTION_L
	BlockData* junctionLBlockData = getBlockData(BlockType::JUNCTION_L);
	junctionLBlockData->setName("Pull-down Resistor");
	junctionLBlockData->setPath("Hi-Z");
	junctionLBlockData->setSize(Size(1, 3));
	junctionLBlockData->setConnectionBidirectional(Vector(0, 2), 0);
	junctionLBlockData->setVirtualConnection(0, 1);
	junctionLBlockData->newRenderData<BlockData::BlockTextureData>();
	junctionLBlockData->setBlockTexturePath(0, mainTexturePath);
	junctionLBlockData->setBlockTextureTopLeft(0, { 0, 4 * 256 });
	junctionLBlockData->setBlockTextureSize(0, { 256, 3 * 256 });
	junctionLBlockData->setBlockTextureVirtualConnection(0, 0);
	junctionLBlockData->setBlockTextureStateOffset(0, { 256, 0 });
	// JUNCTION_H
	BlockData* junctionHBlockData = getBlockData(BlockType::JUNCTION_H);
	junctionHBlockData->setName("Pull-up Resistor");
	junctionHBlockData->setPath("Hi-Z");
	junctionHBlockData->setSize(Size(1, 3));
	junctionHBlockData->setConnectionBidirectional(Vector(0, 2), 0);
	junctionHBlockData->setVirtualConnection(0, 1);
	junctionHBlockData->newRenderData<BlockData::BlockTextureData>();
	junctionHBlockData->setBlockTexturePath(0, mainTexturePath);
	junctionHBlockData->setBlockTextureTopLeft(0, { 4 * 256, 4 * 256 });
	junctionHBlockData->setBlockTextureSize(0, { 256, 3 * 256 });
	junctionHBlockData->setBlockTextureVirtualConnection(0, 0);
	junctionHBlockData->setBlockTextureStateOffset(0, { 256, 0 });
	// JUNCTION_X
	BlockData* junctionXBlockData = getBlockData(BlockType::JUNCTION_X);
	junctionXBlockData->setName("Pull-X Resistor");
	junctionXBlockData->setPath("Hi-Z");
	junctionXBlockData->setSize(Size(1, 3));
	junctionXBlockData->setConnectionBidirectional(Vector(0, 2), 0);
	junctionXBlockData->setVirtualConnection(0, 1);
	junctionXBlockData->newRenderData<BlockData::BlockTextureData>();
	junctionXBlockData->setBlockTexturePath(0, mainTexturePath);
	junctionXBlockData->setBlockTextureTopLeft(0, { 0, 3 * 256 });
	junctionXBlockData->setBlockTextureSize(0, { 256, 3 * 256 });
	junctionXBlockData->setBlockTextureVirtualConnection(0, 0);
	junctionXBlockData->setBlockTextureStateOffset(0, { 256, 0 });
	junctionXBlockData->setIsPlaceable(false);
	// TRISTATE_BUFFER
	BlockData* tristateBufferBlockData = getBlockData(BlockType::TRISTATE_BUFFER);
	tristateBufferBlockData->setName("Tristate Buffer");
	tristateBufferBlockData->setPath("Hi-Z");
	tristateBufferBlockData->setConnectionInput(Vector(0, 1), 0);
	tristateBufferBlockData->setConnectionPortOffset(0, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	tristateBufferBlockData->setConnectionIdName(0, "Data");
	tristateBufferBlockData->setConnectionInput(Vector(0, 0), 1);
	tristateBufferBlockData->setConnectionPortOffset(1, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	tristateBufferBlockData->setConnectionIdName(1, "Enable");
	tristateBufferBlockData->setConnectionOutput(Vector(0, 1), 2);
	tristateBufferBlockData->setConnectionPortOffset(2, FVector(0.5f + edgeDistance, 0.5f + sideShift));
	tristateBufferBlockData->setConnectionIdName(2, "Output");
	tristateBufferBlockData->setSize(Size(1, 2));
	tristateBufferBlockData->setVirtualConnection(0, 1);
	tristateBufferBlockData->newRenderData<BlockData::BlockTextureData>();
	tristateBufferBlockData->setBlockTexturePath(0, mainTexturePath);
	tristateBufferBlockData->setBlockTextureTopLeft(0, { 8 * 256, 4 * 256 });
	tristateBufferBlockData->setBlockTextureSize(0, { 256, 2 * 256 });
	tristateBufferBlockData->setBlockTextureVirtualConnection(0, 0);
	tristateBufferBlockData->setBlockTextureStateOffset(0, { 256, 0 });
	// tristateBufferBlockData->setTexturePath(mainTexturePath);
	// tristateBufferBlockData->setTextureVirtualConnection(0);
	// tristateBufferBlockData->setUsesTileMapTexture(true);
	// tristateBufferBlockData->setTextureTileSize({ 256, 256 });
	// tristateBufferBlockData->setTextureBlockTileSize({ 1, 2 });
	// tristateBufferBlockData->setTextureSmallestCordTile({ 8, 4 });
	// tristateBufferBlockData->setTextureBlockStateOffset({ 256, 0 });
	// COLOR_LIGHT
	// BlockData* colorLightBlockData = getBlockData(BlockType::COLOR_LIGHT);
	// colorLightBlockData->setName("Color Light (6 bits)");
	// colorLightBlockData->setPath("Other");
	// colorLightBlockData->setConnectionInput(Vector(0), 0);
	// colorLightBlockData->setConnectionPortOffset(0, FVector(0.5f - edgeDistance, 0.5f - sideShift));
	// colorLightBlockData->setConnectionBitConfiguration(0, std::vector<unsigned int>{ 0, 1, 2, 3, 4, 5 });
	// colorLightBlockData->setVirtualConnection(0, 6);
	// colorLightBlockData->setTexturePath((DirectoryManager::getResourceDirectory() / "colorLight.png").string());
	// colorLightBlockData->setTextureVirtualConnection(0);
	// colorLightBlockData->setUsesTileMapTexture(true);
	// colorLightBlockData->setTextureTileSize({ 256, 256 });
	// colorLightBlockData->setTextureBlockTileSize({ 1, 1 });
	// colorLightBlockData->setTextureSmallestCordTile({ 0, 0 });
	// colorLightBlockData->setTextureBlockStateOffset({ 8, 256 });
	// colorLightBlockData->setIsPlaceable(false);
	// // TEXTURE_VIEWER
	// BlockData* textureViewerBlockData = getBlockData(addBlock());
	// textureViewerBlockData->setName("Texture Viewer");
	// textureViewerBlockData->setPath("Debug");
	// textureViewerBlockData->setSize(Size(5));
	// textureViewerBlockData->setTexturePath("null_path");
	// textureViewerBlockData->setUsesTileMapTexture(true);
	// textureViewerBlockData->setTextureTileSize({ 4096, 4096 });
	// textureViewerBlockData->setTextureBlockTileSize({ 1, 1 });
	// textureViewerBlockData->setTextureSmallestCordTile({ 0, 0 });
	// textureViewerBlockData->setIsPlaceable(true);
	logInfo("Default BlockData initialized", "BlockDataManager");
}

BlockType BlockDataManager::getBusBlock(unsigned int bitWidth) { return getBusBlock(bitWidth, 1); }

BlockType BlockDataManager::getBusBlock(unsigned int numInputs, unsigned int inputBitwidth) {
	std::vector<BusConnectionData> busConnections;
	for (unsigned int i = 0; i < numInputs; i++) {
		std::vector<unsigned int> bitConfig;
		for (unsigned int j = 0; j < inputBitwidth; j++) {
			bitConfig.push_back(i * inputBitwidth + j);
		}
		busConnections.emplace_back(Vector(0, i), bitConfig);
	}
	busConnections.emplace_back(Vector(1, 0), numInputs * inputBitwidth);
	return getBusBlock(busConnections);
}

BlockType BlockDataManager::getBusBlock(unsigned int numInputs, unsigned int numOutputs, unsigned int inputLaneWidth, unsigned int outputLaneWidth) {
	std::vector<BusConnectionData> busConnections;
	for (unsigned int i = 0; i < numInputs; i++) {
		std::vector<unsigned int> bitConfig;
		for (unsigned int j = i * inputLaneWidth; j < (i + 1) * inputLaneWidth; j++) {
			bitConfig.push_back(j);
		}
		busConnections.emplace_back(Vector(0, i), bitConfig);
	}
	for (unsigned int i = 0; i < numOutputs; i++) {
		std::vector<unsigned int> bitConfig;
		for (unsigned int j = i * outputLaneWidth; j < (i + 1) * outputLaneWidth; j++) {
			bitConfig.push_back(j);
		}
		busConnections.emplace_back(Vector(1, i), bitConfig);
	}
	return getBusBlock(busConnections);
}

BlockType BlockDataManager::getBusBlock(std::vector<BusConnectionData> busConnections) {
	std::sort(busConnections.begin(), busConnections.end(), [](const BusConnectionData& a, const BusConnectionData& b) -> bool {
		return (a.positionOnBlock.dx < b.positionOnBlock.dx) || (a.positionOnBlock.dx == b.positionOnBlock.dx && a.positionOnBlock.dy < b.positionOnBlock.dy);
	});
	for (unsigned int i = 0; i < busConnections.size() - 1; i++) {
		if (busConnections[i].positionOnBlock == busConnections[i + 1].positionOnBlock) {
			logError("Can't have 2 bus ports at the same position {} on a block.", "", busConnections[i].positionOnBlock);
			return BlockType::NONE;
		}
	}
	if (createdBuses.contains(busConnections)) {
		return createdBuses.at(busConnections);
	}

	Size blockSize(1);
	for (const BusConnectionData& busConnection : busConnections) {
		blockSize.extentToFitTartgetCell(busConnection.positionOnBlock);
	}

	BlockType blockType = addBlock();
	createdBuses[busConnections] = blockType;
	BlockData* busInterfaceBlockData = getBlockData(blockType);
	busInterfaceBlockData->setIsBus(true);
	busInterfaceBlockData->setSize(blockSize);
	busInterfaceBlockData->setIsPlaceable(false);
	std::string name = "Bus ";
	unsigned int range = 0;
	for (unsigned int i = 0; i < busConnections.size(); i++) {
		busInterfaceBlockData->setConnectionBidirectional(busConnections[i].positionOnBlock, i);
		if (std::holds_alternative<unsigned int>(busConnections[i].bitConfiguration)) {
			if (range != 0) {
				name += std::to_string(range) + "x1, ";
				range = 0;
			}
			name += "1x" + std::to_string(std::get<unsigned int>(busConnections[i].bitConfiguration));
			busInterfaceBlockData->setConnectionBitConfiguration(i, std::get<unsigned int>(busConnections[i].bitConfiguration));
			if (i + 1 < busConnections.size()) name += ", ";
		} else {
			auto& vec = std::get<std::vector<unsigned int>>(busConnections[i].bitConfiguration);
			if (vec.size() == 1) {
				if (range == vec.back()) {
					range++;
				} else {
					if (range != 0) name += std::to_string(range) + "x1, ";
					range = 1;
				}
			} else {
				if (range != 0) {
					name += std::to_string(range) + "x1, ";
					range = 0;
				}
				name += "{";
				for (unsigned int j = 0; j < vec.size(); j++) {
					name += std::to_string(vec[j]);
					if (j + 1 < vec.size()) {
						name += ",";
					}
				}
				name += "}";
				if (i + 1 < busConnections.size()) name += ", ";
			}
			busInterfaceBlockData->setConnectionBitConfiguration(i, vec);
		}
	}
	if (range != 0) name += std::to_string(range) + "x1, ";
	busInterfaceBlockData->setName(name);
	busInterfaceBlockData->setPath("Other");
	return blockType;
}

nlohmann::json BlockDataManager::dumpState() const /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json stateJson;
	stateJson["blockData"] = nlohmann::json::array();
	for (const BlockData& bd : blockData) {
		stateJson["blockData"].push_back(bd.dumpState());
	}
	stateJson["createdBuses"] = nlohmann::json::array();
	for (const auto& [busConnections, blockType] : createdBuses) {
		nlohmann::json busJson;
		busJson["busConnections"] = dumpBusConnectionDataVector(busConnections);
		busJson["blockType"] = blocktype_to_string(blockType);
		stateJson["createdBuses"].push_back(busJson);
	}
	return stateJson;
}

nlohmann::json BlockDataManager::dumpBusConnectionDataVector(const std::vector<BlockDataManager::BusConnectionData>& busConnections) /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json busConnectionsJson = nlohmann::json::array();
	for (const BusConnectionData& busConnection : busConnections) {
		busConnectionsJson.push_back(busConnection.dumpState());
	}
	return busConnectionsJson;
}

nlohmann::json BlockDataManager::BusConnectionData::dumpState() const /* GCOVR_EXCL_FUNCTION */ {
	nlohmann::json busConnectionJson;
	busConnectionJson["positionOnBlock"] = positionOnBlock.toString();
	if (std::holds_alternative<unsigned int>(bitConfiguration)) {
		busConnectionJson["bitConfiguration"] = std::get<unsigned int>(bitConfiguration);
	} else {
		busConnectionJson["bitConfiguration"] = nlohmann::json::array();
		busConnectionJson["bitConfiguration"] = std::get<std::vector<unsigned int>>(bitConfiguration);
	}
	return busConnectionJson;
}
