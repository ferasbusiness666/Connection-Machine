#ifndef circuitTestWidget_h
#define citcuitTestWidget_h

#include "backend/container/block/blockDefs.h"
#include "gui/mainWindow/widget.h"
#include "backend/circuit/circuitDefs.h"
#include "backend/dataUpdateEventManager.h"

class CircuitView;

class CircuitTestWidget : public Widget {
public:
	CircuitTestWidget(WidgetId widgetId, MainWindow& mainWindow, circuit_id_t circuitId = 0);
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
};

#endif /* circuitTestWidget_h */
