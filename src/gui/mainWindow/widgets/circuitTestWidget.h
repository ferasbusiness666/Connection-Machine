#ifndef circuitTestWidget_h
#define circuitTestWidget_h

#include "backend/container/block/blockDefs.h"
#include "gui/mainWindow/widget.h"
#include "backend/circuit/circuitDefs.h"
#include "backend/dataUpdateEventManager.h"
#include "backend/circuitTests/circuitTestGroup.h"

class CircuitView;

class CircuitTestWidget : public Widget {
public:
	CircuitTestWidget(WidgetId widgetId, MainWindow& mainWindow);
	~CircuitTestWidget();

	void processEvent(SDL_Event& event) override final;

private:
	void render() override final;
	void update() override final;
	void renderViewport(circuit_id_t circuitId);
	void renderSideBar(circuit_id_t circuitId);

	DataUpdateEventManager::DataUpdateEventReceiver dataUpdateEventReceiver;

	BlockType blockType = BlockType::NONE;

	circuit_id_t renderingCircuitId = 0;
	std::unique_ptr<CircuitView> circuitView;
	std::mutex circuitsMux;
	std::map<circuit_id_t, std::string> circuits;
	std::string testGroupName;
	std::mutex testGroupCopyMux;
	CircuitTestGroup::CircuitTestGroupCopy testGroupCopy;
};

#endif /* circuitTestWidget_h */
