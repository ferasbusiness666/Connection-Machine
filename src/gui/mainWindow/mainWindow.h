#ifndef window_h
#define window_h

#include <SDL3/SDL_events.h>

#include "environment/environment.h"
#include "gui/sdl/sdlWindow.h"

#include "tools/toolManagerManager.h"
#include "widget.h"

class MainWindow : public SdlWindow {
public:
	MainWindow();
	~MainWindow();

	// no copy
	MainWindow(const MainWindow&) = delete;
	MainWindow& operator=(const MainWindow&) = delete;

	Environment& getEnvironment() { return environment; }
	const Environment& getEnvironment() const { return environment; }

	ToolManagerManager& getToolManagerManager() { return toolManagerManager; }
	const ToolManagerManager& getToolManagerManager() const { return toolManagerManager; }

	// logging
	// CornerLog& getCornerLog() { return cornerLog.value(); } // dont need this with the other functions here
	void log(const std::string& message) { /*cornerLog->log(message);*/ }
	template <typename... Args>
	void log(const fmt::format_string<Args...>& formatString, Args&&... args) {
		std::ostringstream message;
		message << fmt::format(formatString, std::forward<Args>(args)...);
		// cornerLog->log(message.str());
	}
	void logError(const std::string& message) { /*cornerLog->logError(message);*/ }
	template <typename... Args>
	void logError(const fmt::format_string<Args...>& formatString, Args&&... args) {
		std::ostringstream message;
		message << fmt::format(formatString, std::forward<Args>(args)...);
		// cornerLog->logError(message.str());
	}

	bool isPressingKeybind(const Keybind& keybind, bool repeat = false) const;
	bool isPressingKeybind(const std::string& settingKey, bool repeat = false) const;
	const std::set<ImGuiKey>& getPressedKeys() const { return pressedKeys; }

	ImGuiID getDockMainId() const { return dockMainId; }
	ImGuiID getDockLeftId() const { return dockLeftId; }
	ImGuiID getDockRightId() const { return dockRightId; }
	ImGuiID getDockBottomId() const { return dockBottomId; }

private:
	void doUpdate() override final;
	bool killWindow(bool forced) override final { widgets.clear(); return true; }
	void render() override final;
	void processEvent(SDL_Event& event) override final;
	nlohmann::json dumpState() const override final { return "Main Window"; }

private:
	void offsetUiScale(double delta);
	void applyUiScale(float scale);

	Environment environment;

	std::set<ImGuiKey> pressedKeys;
	std::atomic<unsigned long long> frameIndex;
	unsigned long long lastUpdatedFrame;

	// inputs and tools
	// KeybindHandler keybindHandler;
	ToolManagerManager toolManagerManager;
	float uiScale = 1.0f;
	bool uiScaleSettingUpdateInProgress = false;
	static constexpr double kUiScaleStep = 0.1;
	static constexpr double kUiScaleMin = 0.5;
	static constexpr double kUiScaleMax = 3.0;

	std::unordered_map<WidgetId, std::unique_ptr<Widget>> widgets;
	IdProvider<WidgetId> widgetIdProvider;

	ImGuiID dockMainId;
	ImGuiID dockLeftId;
	ImGuiID dockRightId;
	ImGuiID dockBottomId;

	// widgets
	// std::optional<SelectorWindow> selectorWindow;
	// std::optional<EvalWindow> evalWindow;
	// std::optional<BlockCreationWindow> blockCreationWindow;
	// std::optional<SimControlsManager> simControlsManager;
	// std::optional<SettingsWindow> settingsWindow;
	// std::optional<MenuBar> menuBar;
	// std::optional<CornerLog> cornerLog;
};

#endif /* window_h */
