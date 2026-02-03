#ifndef widget_h
#define widget_h

#include "util/id.h"

DECLARE_ID_TYPE(WidgetId, unsigned int);

class MainWindow;
#include <SDL3/SDL_events.h>

class Widget {
	template<class ValueType> struct GuiValue;
	struct GuiValueBase {
		virtual ~GuiValueBase() = default;
		virtual void renderDataUpdate() = 0;
		template<class ValueType>
		GuiValue<ValueType>* cast() { return dynamic_cast<GuiValue<ValueType>*>(this); }
		template<class ValueType>
		const GuiValue<ValueType>* cast() const { return dynamic_cast<const GuiValue<ValueType>*>(this); }
	};
	template<class ValueType>
	struct GuiValue : public GuiValueBase {
		GuiValue(const ValueType& initalValue, std::function<void(const ValueType&)> func) :
			value(initalValue), renderingValue(initalValue), listener(func) { }
		void renderDataUpdate() override {
			if (value != renderingValue) {
				if (updateFromRendering) {
					value = renderingValue;
					if (listener) listener(value);
				} else {
					renderingValue = value;
				}
			}
			updateFromRendering = true;
		}
		bool updateFromRendering = true;
		ValueType value;
		ValueType renderingValue;
		std::function<void(const ValueType&)> listener;
	};
public:
	Widget(WidgetId widgetId, MainWindow& mainWindow) : widgetId(widgetId), mainWindow(mainWindow), widgetIdStr(fmt::format("Widget {}", widgetId)) { }
	virtual ~Widget() = default;
	void doRendering(std::function<void(std::shared_ptr<void>)> preserveForFrame) {
		render(preserveForFrame);
		doRenderDataUpdate.store(true);
	}
	void doUpdate() {
		if (doRenderDataUpdate.load()) {
			doRenderDataUpdate.store(false);
			for (std::pair<const std::string, std::unique_ptr<GuiValueBase>>& guiValueBase : guiValues) guiValueBase.second->renderDataUpdate();
		}
	}
	MainWindow& getMainWindow() const { return mainWindow; }
	WidgetId getWidgetId() const { return widgetId; }
	const std::string& getWidgetIdStr() const { return widgetIdStr; }
	virtual void processEvent(SDL_Event& event) = 0;

protected:
	virtual void render(std::function<void(std::shared_ptr<void>)> preserveForFrame) = 0;

	// gui values
	template<class ValueType>
	void setupGUIValue(const std::string& key, const ValueType& initalValue, std::function<void(const ValueType&)> func);
	template<class ValueType>
	const std::optional<ValueType>& getGUIValue(const std::string& key) const;
	template<class ValueType>
	void setGUIValue(const std::string& key, const ValueType& value);
	template<class ValueType>
	const std::optional<ValueType>& getGUIValue_rendering(const std::string& key) const;
	template<class ValueType>
	void setGUIValue_rendering(const std::string& key, const ValueType& value);
	template<class ValueType>
	ValueType* getGUIValueForImGui_rendering(const std::string& key);
private:
	MainWindow& mainWindow;
	WidgetId widgetId;
	std::string widgetIdStr;
	mutable std::mutex guiValuesMux;
	std::unordered_map<std::string, std::unique_ptr<GuiValueBase>> guiValues;
	std::atomic<bool> doRenderDataUpdate;
};

template<class ValueType>
void Widget::setupGUIValue(const std::string& key, const ValueType& initalValue, std::function<void(const ValueType&)> func) {
	std::lock_guard mux(guiValuesMux);
	auto iter = guiValues.emplace(key, std::make_unique<GuiValue<ValueType>>(initalValue, func));
	if (iter.second) return;
	return;
}

template<class ValueType>
const std::optional<ValueType>& Widget::getGUIValue(const std::string& key) const {
	std::lock_guard mux(guiValuesMux);
	auto iter = guiValues.find(key);
	if (iter == guiValues.end()) {
		logError("Could not find {} in guiValues.", "Widget::getGUIValue", key);
		return std::nullopt;
	}
	const GuiValue<ValueType>* guiValue = iter->second->cast<ValueType>();
	if (guiValue == nullptr) {
		logError("Could not cast value with key {}.", "Widget::getGUIValue", key);
		return std::nullopt;
	}
	return guiValue->value;
}

template<class ValueType>
void Widget::setGUIValue(const std::string& key, const ValueType& value) {
	std::lock_guard mux(guiValuesMux);
	auto iter = guiValues.find(key);
	if (iter == guiValues.end()) {
		logError("Could not find {} in guiValues.", "Widget::setGUIValue", key);
		return;
	}
	GuiValue<ValueType>* guiValue = iter->second->template cast<ValueType>();
	if (guiValue == nullptr) {
		logError("Trying to change type of key {}.", "Widget::setGUIValue", key);
		return;
	}
	guiValue->updateFromRendering = false;
	guiValue->value = value;
}

template<class ValueType>
const std::optional<ValueType>& Widget::getGUIValue_rendering(const std::string& key) const {
	std::lock_guard mux(guiValuesMux);
	auto iter = guiValues.find(key);
	if (iter == guiValues.end()) {
		logError("Could not find {} in guiValues.", "Widget::getGUIValue_rendering", key);
		return std::nullopt;
	}
	const GuiValue<ValueType>* guiValue = iter->second->cast<ValueType>();
	if (guiValue == nullptr) {
		logError("Could not cast value with key {}.", "Widget::getGUIValue_rendering", key);
		return std::nullopt;
	}
	return &guiValue->renderingValue;
}

template<class ValueType>
void Widget::setGUIValue_rendering(const std::string& key, const ValueType& value) {
	std::lock_guard mux(guiValuesMux);
	auto iter = guiValues.find(key);
	if (iter == guiValues.end()) {
		logError("Could not find {} in guiValues.", "Widget::setGUIValue_rendering", key);
		return;
	}
	GuiValue<ValueType>* guiValue = iter->second->cast<ValueType>();
	if (guiValue == nullptr) {
		logError("Trying to change type of key {}.", "Widget::setGUIValue_rendering", key);
		return;
	}
	guiValue->renderingValue = value;
}

template<class ValueType>
ValueType* Widget::getGUIValueForImGui_rendering(const std::string& key) {
	std::lock_guard mux(guiValuesMux);
	auto iter = guiValues.find(key);
	if (iter == guiValues.end()) {
		logError("Could not find {} in guiValues.", "Widget::getGUIValueForImGui_rendering", key);
		return nullptr;
	}
	GuiValue<ValueType>* guiValue = iter->second->cast<ValueType>();
	if (guiValue == nullptr) {
		logError("Could not cast value with key {}.", "Widget::getGUIValueForImGui_rendering", key);
		return nullptr;
	}
	return &guiValue->renderingValue;
}

#endif /* widget_h */