#ifndef circuitViewWidget_h
#define circuitViewWidget_h

#include "../widget.h"

class Environment;
class CircuitView;

class CircuitViewWidget : public Widget {
public:
	CircuitViewWidget(WidgetId widgetId, MainWindow& mainWindow);
	~CircuitViewWidget();
	void processEvent(SDL_Event& event) override final;
private:
	void render() override final;
	std::unique_ptr<CircuitView> circuitView;
	bool mouseControls;
};

#endif /* circuitViewWidget_h */