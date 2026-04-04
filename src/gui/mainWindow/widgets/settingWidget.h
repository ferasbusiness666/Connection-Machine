#ifndef settingWidget_h
#define settingWidget_h

#include "../widget.h"

class SettingWidget : public Widget {
public:
	SettingWidget(WidgetId widgetId, MainWindow& mainWindow);

private:
	void render() override final;
};

#endif /* settingWidget_h */
