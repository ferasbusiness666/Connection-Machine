#ifndef blockCreationWidget_h
#define blockCreationWidget_h

#include "gui/mainWindow/widget.h"
#include "backend/blockData/blockData.h"
#include "backend/circuit/circuitDefs.h"
#include "backend/dataUpdateEventManager.h"

class CircuitView;

class BlockCreationWidget : public Widget {
public:
	BlockCreationWidget(WidgetId widgetId, MainWindow& mainWindow, circuit_id_t circuitId = 0);
	~BlockCreationWidget();

	void processEvent(SDL_Event& event) override final;

private:
	void render() override final;
	void update() override final;
	void renderSideBar(circuit_id_t circuitId);

	DataUpdateEventManager::DataUpdateEventReceiver dataUpdateEventReceiver;

	BlockType blockType = BlockType::NONE;

	std::unique_ptr<CircuitView> circuitView;
	std::mutex circuitsMux;
	std::map<circuit_id_t, std::string> circuits;
	std::mutex blockDataCopyMux;
	std::optional<BlockData::BlockDataCopy> blockDataCopy;
};

#endif /* blockCreationWidget_h */
