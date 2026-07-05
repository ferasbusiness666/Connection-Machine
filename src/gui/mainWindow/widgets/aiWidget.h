#ifndef aiWidget_h
#define aiWidget_h

#include "../widget.h"
#include "backend/ai/aiService.h"
#include <deque>
#include <string>
#include <vector>
#include <atomic>

class AIWidget : public Widget {
public:
    AIWidget(WidgetId widgetId, MainWindow& mainWindow);
    ~AIWidget();

    // Entry point for AI to modify circuits
    void applyAIAction(const std::string& action, const std::string& params);

private:
    void render() override final;
    void update() override final;
    nlohmann::json dumpState() const override final;

    // UI rendering helpers
    void renderToolbar();
    void renderChatArea();
    void renderInputArea();
    void renderSettingsPanel();
    void renderProviderSettings();
    void renderDiffPanel();
    void renderCircuitContext();

    // AI interaction
    void sendMessage();
    void handleAIResponse(const AIService::ChatResponse& response);
    void addSystemMessage(const std::string& content);

    // Circuit context
    std::string buildCircuitContextString();
    std::string buildSystemPrompt();
    std::string getCurrentCircuitState();

    // Panel state
    bool showSettings_ = false;
    bool showDiffPanel_ = false;
    bool showContextPanel_ = false;
    bool isLoading_ = false;
    bool streamingResponse_ = false;

    // Provider selection
    AIService::Provider currentProvider_ = AIService::Provider::GROQ;
    std::string currentModel_;
    std::vector<AIService::Model> availableModels_;
    bool modelsLoaded_ = false;

    // Chat
    struct ChatEntry {
        std::string role;   // "user", "assistant", "system", "error"
        std::string content;
        double timestamp = 0;
    };
    std::deque<ChatEntry> chatHistory_;
    std::string inputBuffer_;
    std::string lastResponse_;

    // Settings
    struct ProviderSettings {
        std::string apiKey;
        bool enabled = false;
        std::string defaultModel;
        bool showKey = false;
    };
    std::array<ProviderSettings, AIService::PROVIDER_COUNT> providerSettings_;

    // Diff panel
    struct DiffEntry {
        std::string type; // "add", "remove", "modify"
        std::string description;
        std::string details;
    };
    std::vector<DiffEntry> pendingDiffs_;

    // Auto-scroll
    bool autoScroll_ = true;

    // Animation / state
    float settingsAnim_ = 0.0f;
    std::atomic<bool> needsModelRefresh_{false};

    // Logging
    std::string errorLog_;
};

#endif // aiWidget_h