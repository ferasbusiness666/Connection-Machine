#ifndef widget_h
#define widget_h

#include "util/id.h"

DECLARE_ID_TYPE(WidgetId, unsigned int);

class MainWindow;

class Widget {
public:
	Widget(WidgetId widgetId, MainWindow& mainWindow) : widgetId(widgetId), mainWindow(mainWindow), widgetIdStr(fmt::format("Widget {}", widgetId)) { }
	virtual ~Widget() = default;
	virtual void render(std::function<void(std::shared_ptr<void>)> preserveForFrame) = 0;
	MainWindow& getMainWindow() const { return mainWindow; }
	WidgetId getWidgetId() const { return widgetId; }
	const std::string& getWidgetIdStr() const { return widgetIdStr; }
private:
	MainWindow& mainWindow;
	WidgetId widgetId;
	std::string widgetIdStr;
};

#endif /* widget_h */