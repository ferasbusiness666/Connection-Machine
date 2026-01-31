#ifndef circuitViewWidget_h
#define circuitViewWidget_h

#include "../widget.h"

class Environment;
class CircuitView;

class CircuitViewWidget : public Widget {
public:
	CircuitViewWidget(WidgetId widgetId, MainWindow& mainWindow);
	~CircuitViewWidget();
	void render() override final;
private:
	std::unique_ptr<CircuitView> circuitView;
};

#endif /* circuitViewWidget_h */