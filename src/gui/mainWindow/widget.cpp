#include "widget.h"

#include "mainWindow.h"

Environment& Widget::getEnvironment() const { return mainWindow.getEnvironment(); }
Backend& Widget::getBackend() const { return mainWindow.getEnvironment().getBackend(); }
