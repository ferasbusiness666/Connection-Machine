#include "blockDataManager.h"

#include "computerAPI/directoryManager.h"

void BlockDataManager::initializeDefaults() {
	assert(blockData.size() == 0); // should call this before doing anything
	// load default data
	for (unsigned int i = 0; i < 21; i++) addBlock();

	std::string mainTexturePath = (DirectoryManager::getResourceDirectory() / "logicTiles.png").string();

	// AND
	BlockData* andBlockData = getBlockData(BlockType::AND);
	andBlockData->setName("And");
	andBlockData->setTexturePath(mainTexturePath);
	andBlockData->setUsesTileMapTexture(true);
	andBlockData->setTextureTileSize({ 256, 256 });
	andBlockData->setTextureBlockTileSize({ 1, 1 });
	andBlockData->setTextureSmallestCordTile({ 1, 0 });
	// OR
	BlockData* orBlockData = getBlockData(BlockType::OR);
	orBlockData->setName("Or");
	orBlockData->setTexturePath(mainTexturePath);
	orBlockData->setUsesTileMapTexture(true);
	orBlockData->setTextureTileSize({ 256, 256 });
	orBlockData->setTextureBlockTileSize({ 1, 1 });
	orBlockData->setTextureSmallestCordTile({ 2, 0 });
	// XOR
	BlockData* xorBlockData = getBlockData(BlockType::XOR);
	xorBlockData->setName("Xor");
	xorBlockData->setTexturePath(mainTexturePath);
	xorBlockData->setUsesTileMapTexture(true);
	xorBlockData->setTextureTileSize({ 256, 256 });
	xorBlockData->setTextureBlockTileSize({ 1, 1 });
	xorBlockData->setTextureSmallestCordTile({ 3, 0 });
	// NAND
	BlockData* nandBlockData = getBlockData(BlockType::NAND);
	nandBlockData->setName("Nand");
	nandBlockData->setTexturePath(mainTexturePath);
	nandBlockData->setUsesTileMapTexture(true);
	nandBlockData->setTextureTileSize({ 256, 256 });
	nandBlockData->setTextureBlockTileSize({ 1, 1 });
	nandBlockData->setTextureSmallestCordTile({ 4, 0 });
	// NOR
	BlockData* norBlockData = getBlockData(BlockType::NOR);
	norBlockData->setName("Nor");
	norBlockData->setTexturePath(mainTexturePath);
	norBlockData->setUsesTileMapTexture(true);
	norBlockData->setTextureTileSize({ 256, 256 });
	norBlockData->setTextureBlockTileSize({ 1, 1 });
	norBlockData->setTextureSmallestCordTile({ 5, 0 });
	// XNOR
	BlockData* xnorBlockData = getBlockData(BlockType::XNOR);
	xnorBlockData->setName("Xnor");
	xnorBlockData->setTexturePath(mainTexturePath);
	xnorBlockData->setUsesTileMapTexture(true);
	xnorBlockData->setTextureTileSize({ 256, 256 });
	xnorBlockData->setTextureBlockTileSize({ 1, 1 });
	xnorBlockData->setTextureSmallestCordTile({ 6, 0 });
	// BUFFER
	BlockData* bufferBlockData = getBlockData(BlockType::BUFFER);
	bufferBlockData->setName("Buffer");
	bufferBlockData->setTexturePath(mainTexturePath);
	bufferBlockData->setUsesTileMapTexture(true);
	bufferBlockData->setTextureTileSize({ 256, 256 });
	bufferBlockData->setTextureBlockTileSize({ 1, 1 });
	bufferBlockData->setTextureSmallestCordTile({ 7, 0 });
	// NOT
	BlockData* notBlockData = getBlockData(BlockType::NOT);
	notBlockData->setName("Not");
	notBlockData->setTexturePath(mainTexturePath);
	notBlockData->setUsesTileMapTexture(true);
	notBlockData->setTextureTileSize({ 256, 256 });
	notBlockData->setTextureBlockTileSize({ 1, 1 });
	notBlockData->setTextureSmallestCordTile({ 8, 0 });
	// JUNCTION
	BlockData* junctionBlockData = getBlockData(BlockType::JUNCTION);
	junctionBlockData->setName("Junction");
	junctionBlockData->setTexturePath(mainTexturePath);
	junctionBlockData->setUsesTileMapTexture(true);
	junctionBlockData->setTextureTileSize({ 256, 256 });
	junctionBlockData->setTextureBlockTileSize({ 1, 1 });
	junctionBlockData->setTextureSmallestCordTile({ 9, 0 });
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
	tristateBufferBlockData->setTextureTileSize({ 256, 256 });
	tristateBufferBlockData->setTextureBlockTileSize({ 1, 2 });
	tristateBufferBlockData->setTextureSmallestCordTile({ 10, 0 });
	// BUTTON
	BlockData* buttonBlockData = getBlockData(BlockType::BUTTON);
	buttonBlockData->setName("Button");
	buttonBlockData->setDefaultData(false);
	buttonBlockData->setConnectionOutput(Vector(0), 0);
	buttonBlockData->setTexturePath(mainTexturePath);
	buttonBlockData->setUsesTileMapTexture(true);
	buttonBlockData->setTextureTileSize({ 256, 256 });
	buttonBlockData->setTextureBlockTileSize({ 1, 1 });
	buttonBlockData->setTextureSmallestCordTile({ 11, 0 });
	// TICK_BUTTON
	BlockData* tickButtonBlockData = getBlockData(BlockType::TICK_BUTTON);
	tickButtonBlockData->setName("Tick Button");
	tickButtonBlockData->setDefaultData(false);
	tickButtonBlockData->setConnectionOutput(Vector(0), 0);
	tickButtonBlockData->setTexturePath(mainTexturePath);
	tickButtonBlockData->setUsesTileMapTexture(true);
	tickButtonBlockData->setTextureTileSize({ 256, 256 });
	tickButtonBlockData->setTextureBlockTileSize({ 1, 1 });
	tickButtonBlockData->setTextureSmallestCordTile({ 12, 0 });
	// SWITCH
	BlockData* switchBlockData = getBlockData(BlockType::SWITCH);
	switchBlockData->setName("Switch");
	switchBlockData->setDefaultData(false);
	switchBlockData->setConnectionOutput(Vector(0), 0);
	switchBlockData->setTexturePath(mainTexturePath);
	switchBlockData->setUsesTileMapTexture(true);
	switchBlockData->setTextureTileSize({ 256, 256 });
	switchBlockData->setTextureBlockTileSize({ 1, 1 });
	switchBlockData->setTextureSmallestCordTile({ 13, 0 });
	// CONSTANT OFF
	BlockData* constantOffBlockData = getBlockData(BlockType::CONSTANT_OFF);
	constantOffBlockData->setName("Constant");
	constantOffBlockData->setDefaultData(false);
	constantOffBlockData->setConnectionOutput(Vector(0), 0);
	constantOffBlockData->setTexturePath(mainTexturePath);
	constantOffBlockData->setUsesTileMapTexture(true);
	constantOffBlockData->setTextureTileSize({ 256, 256 });
	constantOffBlockData->setTextureBlockTileSize({ 1, 1 });
	constantOffBlockData->setTextureSmallestCordTile({ 14, 0 });
	// constantOffBlockDataON
	BlockData* constantOnBlockData = getBlockData(BlockType::CONSTANT_ON);
	constantOnBlockData->setName("Constant");
	constantOnBlockData->setDefaultData(false);
	constantOnBlockData->setIsPlaceable(false);
	constantOnBlockData->setConnectionOutput(Vector(0), 0);
	constantOnBlockData->setTexturePath(mainTexturePath);
	constantOnBlockData->setUsesTileMapTexture(true);
	constantOnBlockData->setTextureTileSize({ 256, 256 });
	constantOnBlockData->setTextureBlockTileSize({ 1, 1 });
	constantOnBlockData->setTextureSmallestCordTile({ 14, 0 });
	// CONSTANT Z
	BlockData* constantZBlockData = getBlockData(BlockType::CONSTANT_Z);
	constantZBlockData->setName("Constant");
	constantZBlockData->setDefaultData(false);
	constantZBlockData->setIsPlaceable(false);
	constantZBlockData->setConnectionOutput(Vector(0), 0);
	constantZBlockData->setTexturePath(mainTexturePath);
	constantZBlockData->setUsesTileMapTexture(true);
	constantZBlockData->setTextureTileSize({ 256, 256 });
	constantZBlockData->setTextureBlockTileSize({ 1, 1 });
	constantZBlockData->setTextureSmallestCordTile({ 14, 0 });
	// CONSTANT X
	BlockData* constantXBlockData = getBlockData(BlockType::CONSTANT_X);
	constantXBlockData->setName("Constant");
	constantXBlockData->setDefaultData(false);
	constantXBlockData->setIsPlaceable(false);
	constantXBlockData->setConnectionOutput(Vector(0), 0);
	constantXBlockData->setTexturePath(mainTexturePath);
	constantXBlockData->setUsesTileMapTexture(true);
	constantXBlockData->setTextureTileSize({ 256, 256 });
	constantXBlockData->setTextureBlockTileSize({ 1, 1 });
	constantXBlockData->setTextureSmallestCordTile({ 14, 0 });
	// LIGHT
	BlockData* lightBlockData = getBlockData(BlockType::LIGHT);
	lightBlockData->setName("Light");
	lightBlockData->setDefaultData(false);
	lightBlockData->setConnectionInput(Vector(0), 0);
	lightBlockData->setTexturePath(mainTexturePath);
	lightBlockData->setUsesTileMapTexture(true);
	lightBlockData->setTextureTileSize({ 256, 256 });
	lightBlockData->setTextureBlockTileSize({ 1, 1 });
	lightBlockData->setTextureSmallestCordTile({ 15, 0 });
	// BUS_INTERFACE_1
	BlockData* busInterfaceBlockData1 = getBlockData(BlockType::BUS_INTERFACE_1);
	busInterfaceBlockData1->setName("Bus Interface 8 -> 1x8");
	busInterfaceBlockData1->setDefaultData(false);
	busInterfaceBlockData1->setIsBus(true);
	busInterfaceBlockData1->setSize(Size(2, 8));
	busInterfaceBlockData1->setConnectionOutput(Vector(1, 0), 0);
	busInterfaceBlockData1->setConnectionBitConfiguration(0, std::vector<unsigned int>{ 0, 1, 2, 3, 4, 5, 6, 7 });
	busInterfaceBlockData1->setConnectionInput(Vector(1, 0), 1);
	busInterfaceBlockData1->setConnectionBitConfiguration(1, std::vector<unsigned int>{ 0, 1, 2, 3, 4, 5, 6, 7 });
	for (unsigned int i = 0; i < 8; ++i) {
		busInterfaceBlockData1->setConnectionOutput(Vector(0, i), i * 2 + 2);
		busInterfaceBlockData1->setConnectionBitConfiguration(i * 2 + 2, std::vector<unsigned int>{ i });
		busInterfaceBlockData1->setConnectionInput(Vector(0, i), i * 2 + 3);
		busInterfaceBlockData1->setConnectionBitConfiguration(i * 2 + 3, std::vector<unsigned int>{ i });
	}
	busInterfaceBlockData1->setTexturePath((DirectoryManager::getResourceDirectory() / "gateIcon.png").string());
	busInterfaceBlockData1->setIsPlaceable(false);
	// BUS_INTERFACE_2
	BlockData* busInterfaceBlockData2 = getBlockData(BlockType::BUS_INTERFACE_2);
	busInterfaceBlockData2->setName("Bus Interface 4x2 -> 1x8");
	busInterfaceBlockData2->setDefaultData(false);
	busInterfaceBlockData2->setIsBus(true);
	busInterfaceBlockData2->setSize(Size(2, 4));
	busInterfaceBlockData2->setConnectionOutput(Vector(1, 0), 0);
	busInterfaceBlockData2->setConnectionBitConfiguration(0, std::vector<unsigned int>{ 0, 1, 2, 3, 4, 5, 6, 7 });
	busInterfaceBlockData2->setConnectionInput(Vector(1, 0), 1);
	busInterfaceBlockData2->setConnectionBitConfiguration(1, std::vector<unsigned int>{ 0, 1, 2, 3, 4, 5, 6, 7 });
	for (unsigned int i = 0; i < 4; ++i) {
		busInterfaceBlockData2->setConnectionOutput(Vector(0, i), i * 2 + 2);
		busInterfaceBlockData2->setConnectionBitConfiguration(i * 2 + 2, std::vector<unsigned int>{ i * 2, i * 2 + 1 });
		busInterfaceBlockData2->setConnectionInput(Vector(0, i), i * 2 + 3);
		busInterfaceBlockData2->setConnectionBitConfiguration(i * 2 + 3, std::vector<unsigned int>{ i * 2, i * 2 + 1 });
	}
	busInterfaceBlockData2->setTexturePath((DirectoryManager::getResourceDirectory() / "gateIcon.png").string());
	busInterfaceBlockData2->setIsPlaceable(false);
	// BUS_INTERFACE_3
	BlockData* busInterfaceBlockData3 = getBlockData(BlockType::BUS_INTERFACE_3);
	busInterfaceBlockData3->setName("Bus Interface 2 -> 1x2");
	busInterfaceBlockData3->setDefaultData(false);
	busInterfaceBlockData3->setIsBus(true);
	busInterfaceBlockData3->setSize(Size(2, 2));
	busInterfaceBlockData3->setConnectionOutput(Vector(1, 0), 0);
	busInterfaceBlockData3->setConnectionBitConfiguration(0, std::vector<unsigned int>{ 0, 1 });
	busInterfaceBlockData3->setConnectionInput(Vector(1, 0), 1);
	busInterfaceBlockData3->setConnectionBitConfiguration(1, std::vector<unsigned int>{ 0, 1 });
	for (unsigned int i = 0; i < 2; ++i) {
		busInterfaceBlockData3->setConnectionOutput(Vector(0, i), i * 2 + 2);
		busInterfaceBlockData3->setConnectionBitConfiguration(i * 2 + 2, std::vector<unsigned int>{ i });
		busInterfaceBlockData3->setConnectionInput(Vector(0, i), i * 2 + 3);
		busInterfaceBlockData3->setConnectionBitConfiguration(i * 2 + 3, std::vector<unsigned int>{ i });
	}
	busInterfaceBlockData3->setTexturePath((DirectoryManager::getResourceDirectory() / "gateIcon.png").string());
	busInterfaceBlockData3->setIsPlaceable(false);
}
