#ifndef blockCreationWidget_h
#define blockCreationWidget_h

#include "../widget.h"
#include "backend/blockData/blockData.h"
#include "backend/circuit/circuitDefs.h"
#include "backend/dataUpdateEventManager.h"

class BlockCreationWidget : public Widget {
public:
	BlockCreationWidget(WidgetId widgetId, MainWindow& mainWindow, circuit_id_t circuitId = 0);
private:
	void render() override final;

	DataUpdateEventManager::DataUpdateEventReceiver dataUpdateEventReceiver;

	BlockType blockType = BlockType::NONE;

	std::mutex circuitsMux;
	std::map<circuit_id_t, std::string> circuits;
	std::mutex renderDataRowsMux;
	std::vector<BlockData::RenderDataType> renderDataRows;
};

#endif /* blockCreationWidget_h */
