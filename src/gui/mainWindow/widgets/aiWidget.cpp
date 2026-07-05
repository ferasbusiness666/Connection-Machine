#include "aiWidget.h"

#include "app.h"
#include "gui/mainWindow/mainWindow.h"
#include "backend/backend.h"
#include "backend/circuit/circuitManager.h"
#include "backend/circuit/circuit.h"
#include "backend/container/blockContainer.h"
#include "backend/container/block/block.h"
#include "backend/blockData/blockDataManager.h"
#include "backend/blockData/blockData.h"

#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "util/preprocessors.h"

#include <sstream>
#include <algorithm>
#include <cctype>

static const ImVec4 COLOR_USER(0.39f, 0.58f, 0.93f, 1.0f);
static const ImVec4 COLOR_ASSISTANT(0.29f, 0.69f, 0.31f, 1.0f);
static const ImVec4 COLOR_SYSTEM(0.69f, 0.69f, 0.29f, 1.0f);
static const ImVec4 COLOR_ERROR(0.93f, 0.29f, 0.29f, 1.0f);
static const ImVec4 COLOR_BG_CHAT(0.08f, 0.08f, 0.10f, 1.0f);
static const ImVec4 COLOR_BG_INPUT(0.12f, 0.12f, 0.15f, 1.0f);

AIWidget::AIWidget(WidgetId widgetId, MainWindow& mainWindow)
    : Widget(widgetId, mainWindow) {
    // Initialize provider settings from AIService
    for (int i = 0; i < AIService::PROVIDER_COUNT; i++) {
        auto provider = static_cast<AIService::Provider>(i);
        const auto& config = AIService::getInstance().getProviderConfig(provider);
        providerSettings_[i].apiKey = config.apiKey;
        providerSettings_[i].enabled = config.enabled;
        providerSettings_[i].defaultModel = config.defaultModel;
    }

    // Set up GUI values
    setupGUIValue<std::string>("aiInput", "", nullptr);
    setupGUIValue<bool>("aiLoading", false, nullptr);

    // Find first enabled provider and set as current
    auto enabled = AIService::getInstance().getEnabledProviders();
    if (!enabled.empty()) {
        currentProvider_ = enabled[0];
        const auto& config = AIService::getInstance().getProviderConfig(currentProvider_);
        currentModel_ = config.defaultModel;
    }

    // Add initial system message
    chatHistory_.push_back({"system",
        "AI Circuit Assistant ready. I can help you design and edit digital logic circuits. "
        "Describe what you want to build or modify.",
        0});

    errorLog_ = "";
}

AIWidget::~AIWidget() = default;

std::string AIWidget::buildSystemPrompt() {
    std::ostringstream prompt;

    prompt << R"(You are an AI assistant for the Connection Machine digital logic circuit simulator.
You can help users design and modify circuits by generating procedural circuit code.

CAPABILITIES:
- Generate new circuits from descriptions
- Modify existing circuits (add/remove blocks, create/remove connections)
- Suggest circuit optimizations and improvements
- Explain circuit behavior

RULES:
1. When generating circuits, describe the blocks, positions, and connections clearly
2. When modifying circuits, reference specific block positions and connection points
3. Keep circuits clean and well-organized - place blocks with spacing
4. Use standard gate types: AND, OR, XOR, NOT, NAND, NOR, SWITCH, LIGHT, CONSTANT_HIGH, CONSTANT_LOW
5. Blocks are placed on a grid - use integer coordinates
6. Connections go from output ports to input ports

CIRCUIT CONTEXT:
)";
    prompt << getCurrentCircuitState();

    return prompt.str();
}

std::string AIWidget::getCurrentCircuitState() {
    // Get the current circuit from the circuit view
    auto& backend = getMainWindow().getEnvironment().getBackend();
    auto& circuitManager = backend.getCircuitManager();

    std::string state;
    bool hasCircuit = false;

    // Get all circuits and find the one that's open
    for (const auto& [id, circuit] : circuitManager.getCircuits()) {
        if (!circuit.closed()) {
            const auto& container = circuit.getBlockContainer();
            state += "Circuit: " + circuit.getCircuitName() + "\n";
            state += "Blocks (" + std::to_string(container.getBlockCount()) + "):\n";

            for (const auto& [blockId, block] : container) {
                auto pos = block.getPosition();
                auto* blockData = container.getBlockDataManager().getBlockData(block.type());
                std::string typeName = blockData ? blockData->getName() : "Unknown";

                state += "  [" + std::to_string(blockId) + "] " + typeName +
                         " at (" + std::to_string(pos.x) + "," + std::to_string(pos.y) + ")\n";
            }

            state += "Connections:\n";
            for (const auto& [blockId, block] : container) {
                const auto& connContainer = block.getConnectionContainer();
                for (const auto& [endId, connections] : connContainer.getConnections()) {
                    for (const auto& connEnd : connections) {
                        const Block* otherBlock = container.getBlock(connEnd.getBlockId());
                        if (otherBlock) {
                            auto otherPos = otherBlock->getPosition();
                            state += "  Block " + std::to_string(blockId) + ":" +
                                     std::to_string(endId.get()) + " -> Block " +
                                     std::to_string(connEnd.getBlockId()) + ":" +
                                     std::to_string(connEnd.getConnectionId().get()) +
                                     " at (" + std::to_string(otherPos.x) + "," + std::to_string(otherPos.y) + ")\n";
                        }
                    }
                }
            }
            hasCircuit = true;
            break;
        }
    }

    if (!hasCircuit) {
        state = "No circuit currently open. You should describe what circuit to create.";
    }

    return state;
}

void AIWidget::sendMessage() {
    if (inputBuffer_.empty() || isLoading_) return;

    std::string userMessage = inputBuffer_;
    inputBuffer_.clear();
    isLoading_ = true;
    setGUIValue<bool>("aiLoading", true);

    // Add user message to chat
    chatHistory_.push_back({"user", userMessage, 0});

    // Build messages for API
    AIService::ChatRequest request;
    request.messages.push_back({"system", buildSystemPrompt()});
    // Add last few messages for context (up to 10)
    int contextCount = 0;
    for (auto it = chatHistory_.rbegin(); it != chatHistory_.rend() && contextCount < 10; ++it, ++contextCount) {
        if (it->role != "system") {
            request.messages.push_back({it->role, it->content});
        }
    }
    // Put them in chronological order
    std::reverse(request.messages.begin() + 1, request.messages.end());
    request.model = currentModel_;
    request.temperature = 0.7;
    request.maxTokens = 4096;

    AIService::getInstance().sendChatRequest(request, currentProvider_,
        [this](const AIService::ChatResponse& response) {
            App::runOnMain([this, response]() {
                handleAIResponse(response);
            });
        });
}

void AIWidget::handleAIResponse(const AIService::ChatResponse& response) {
    isLoading_ = false;
    setGUIValue<bool>("aiLoading", false);

    if (response.success) {
        chatHistory_.push_back({"assistant", response.content, 0});
        lastResponse_ = response.content;

        // Try to parse the response for circuit actions
        // Look for structured commands in the response
        std::string lowerContent = response.content;
        std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);

        // Add a friendly note about what was done
        if (lowerContent.find("```") != std::string::npos) {
            chatHistory_.push_back({"system",
                "I've included code in my response. You can apply the changes using the Apply button.",
                0});
        }
    } else {
        chatHistory_.push_back({"error",
            "Error: " + response.error +
            "\n\nPlease check:\n- Your API key is correct\n- The provider is enabled\n- Your internet connection\n- The model is available",
            0});
        errorLog_ = "API Error: " + response.error;
    }
}

void AIWidget::applyAIAction(const std::string& action, const std::string& params) {
    // This will be called when the user clicks "Apply" on AI suggestions
    // Parse the action and apply circuit changes
    auto& backend = getMainWindow().getEnvironment().getBackend();
    auto& circuitManager = backend.getCircuitManager();

    // Find the current circuit
    for (auto& [id, circuit] : circuitManager.getCircuits()) {
        if (!circuit.closed()) {
            // TODO: Parse AI-generated circuit description and apply changes
            // For now, this is a placeholder for the integration
            chatHistory_.push_back({"system",
                "Changes applied to circuit: " + circuit.getCircuitName(), 0});
            break;
        }
    }
}

// ================ Render Methods ================

void AIWidget::render() {
    ImGui::SetNextWindowDockID(getMainWindow().getDockMainId(), ImGuiCond_FirstUseEver);
    getMainWindow().setNextWindowSideBarDockable();
    getMainWindow().pushWindowStyling();

    bool open = true;
    ifGui (ImGui::Begin(("AI###" + getWidgetIdStr()).c_str(), &open,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar),
        getMainWindow().popWindowStyling();
    ) {
        renderToolbar();

        if (showSettings_) {
            renderSettingsPanel();
        } else {
            ImGui::Separator();
            renderCircuitContext();
            ImGui::Separator();

            // Chat display area
            renderChatArea();

            // Input area at bottom
            renderInputArea();
        }
    }

    if (!open) {
        getMainWindow().destroyWidget(getWidgetId());
    }
    ImGui::End();

    // Handle model refresh needs
    if (needsModelRefresh_.exchange(false)) {
        availableModels_ = AIService::getInstance().getModels(currentProvider_);
    }
}

void AIWidget::renderToolbar() {
    if (ImGui::BeginMenuBar()) {
        // Provider selector
        auto enabledProviders = AIService::getInstance().getEnabledProviders();
        if (!enabledProviders.empty()) {
            const char* preview = AIService::getProviderName(currentProvider_).c_str();
            if (ImGui::BeginCombo("##provider", preview, ImGuiComboFlags_NoArrowButton)) {
                for (auto provider : enabledProviders) {
                    bool isSelected = (provider == currentProvider_);
                    std::string name = AIService::getProviderName(provider);
                    if (ImGui::Selectable(name.c_str(), isSelected)) {
                        currentProvider_ = provider;
                        const auto& config = AIService::getInstance().getProviderConfig(provider);
                        currentModel_ = config.defaultModel;
                        availableModels_ = AIService::getInstance().getModels(provider);
                        modelsLoaded_ = !availableModels_.empty();
                        if (!modelsLoaded_) {
                            AIService::getInstance().fetchModels(provider,
                                [this](std::vector<AIService::Model> models) {
                                    App::runOnMain([this, models]() {
                                        availableModels_ = models;
                                        modelsLoaded_ = true;
                                        if (!models.empty() && currentModel_.empty()) {
                                            currentModel_ = models[0].id;
                                        }
                                    });
                                });
                        }
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                    // Show provider name in fainter text
                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", name.c_str());
                }
                ImGui::EndCombo();
            }

            // Model selector
            if (!availableModels_.empty()) {
                ImGui::SameLine();
                const char* modelPreview = currentModel_.c_str();
                if (ImGui::BeginCombo("##model", modelPreview, ImGuiComboFlags_NoArrowButton)) {
                    for (const auto& model : availableModels_) {
                        bool isSelected = (model.id == currentModel_);
                        if (ImGui::Selectable(model.name.c_str(), isSelected)) {
                            currentModel_ = model.id;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                        // Show provider name faintly beside each model
                        if (enabledProviders.size() > 1) {
                            ImGui::SameLine();
                            ImGui::TextDisabled("%s",
                                AIService::getProviderName(model.provider).c_str());
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        } else {
            ImGui::TextDisabled("No providers configured");
        }

        // Spacer
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 60);

        // Settings button (top right)
        if (ImGui::Button(showSettings_ ? "Chat" : "Settings")) {
            showSettings_ = !showSettings_;
        }

        ImGui::EndMenuBar();
    }
}

void AIWidget::renderChatArea() {
    ImVec2 contentArea = ImGui::GetContentRegionAvail();
    float inputHeight = 100.0f;
    float contextHeight = showContextPanel_ ? 80.0f : 0.0f;
    ImVec2 chatSize(contentArea.x, contentArea.y - inputHeight - contextHeight - 10);

    if (chatSize.y < 50) chatSize.y = 50;

    ImGui::BeginChild("ChatArea", chatSize, false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    // Render chat history
    for (const auto& entry : chatHistory_) {
        ImVec4 color;
        const char* prefix;
        if (entry.role == "user") {
            color = COLOR_USER;
            prefix = "You";
        } else if (entry.role == "assistant") {
            color = COLOR_ASSISTANT;
            prefix = "AI";
        } else if (entry.role == "error") {
            color = COLOR_ERROR;
            prefix = "Error";
        } else {
            color = COLOR_SYSTEM;
            prefix = "System";
        }

        // Message bubble
        ImGui::PushStyleColor(ImGuiCol_ChildBg, COLOR_BG_CHAT);
        ImGui::BeginChild(("msg_" + std::to_string(reinterpret_cast<uintptr_t>(&entry))).c_str(),
            ImVec2(ImGui::GetContentRegionAvail().x, 0), true);

        ImGui::TextColored(color, "%s", prefix);
        ImGui::SameLine();
        ImGui::TextDisabled("%s", "");
        ImGui::TextWrapped("%s", entry.content.c_str());

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    if (isLoading_) {
        ImGui::TextColored(COLOR_ASSISTANT, "AI is thinking");
        // Simple loading dots animation
        float t = ImGui::GetTime();
        int dots = (int)(t * 2) % 4;
        for (int i = 0; i < dots; i++) ImGui::SameLine();
        ImGui::TextDisabled("...");
    }

    if (autoScroll_) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

void AIWidget::renderInputArea() {
    ImVec2 contentArea = ImGui::GetContentRegionAvail();
    float contextHeight = showContextPanel_ ? 80.0f : 0.0f;
    ImVec2 inputSize(contentArea.x, contentArea.y - contextHeight - 10);

    if (inputSize.y < 50) inputSize.y = 50;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, COLOR_BG_INPUT);
    ImGui::BeginChild("InputArea", ImVec2(inputSize.x, inputSize.y - 30), true);

    // Multi-line text input
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (isLoading_) flags |= ImGuiInputTextFlags_ReadOnly;

    if (ImGui::InputTextMultiline("##input", &inputBuffer_, ImVec2(-FLT_MIN, -FLT_MIN), flags)) {
        // Enter pressed - send message
        if (!inputBuffer_.empty()) {
            sendMessage();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // Bottom buttons
    ImGui::BeginChild("InputButtons", ImVec2(0, 30), false);

    // Left side: Send button
    if (ImGui::Button(isLoading_ ? "..." : "Send", ImVec2(60, 0)) && !isLoading_ && !inputBuffer_.empty()) {
        sendMessage();
    }

    // Center: Auto-scroll toggle
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll_);
    ImGui::SameLine();
    ImGui::Checkbox("Context", &showContextPanel_);

    // Right side: Clear chat
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80);
    if (ImGui::Button("Clear Chat")) {
        chatHistory_.clear();
        chatHistory_.push_back({"system", "Chat cleared. Ready for new requests.", 0});
    }

    ImGui::EndChild();
}

void AIWidget::renderSettingsPanel() {
    // Settings panel with provider configuration
    ImGui::Text("AI Provider Settings");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("SettingsScroll", ImVec2(0, ImGui::GetContentRegionAvail().y - 30), false);

    for (int i = 0; i < AIService::PROVIDER_COUNT; i++) {
        auto provider = static_cast<AIService::Provider>(i);
        std::string name = AIService::getProviderName(provider);
        std::string key = AIService::getProviderKey(provider);

        auto& settings = providerSettings_[i];
        auto& config = AIService::getInstance().getProviderConfig(provider);

        ImGui::PushID(key.c_str());

        // Collapsible header for each provider
        bool open = ImGui::TreeNodeEx(name.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed);

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 40);
        ImGui::TextDisabled(settings.enabled ? "Connected" : "Disconnected");

        if (open) {
            ImGui::Indent(10.0f);

            // API Key
            ImGui::Text("API Key");
            ImGui::SameLine();
            ImGui::TextDisabled("(stored locally)");
            ImGui::PushItemWidth(-FLT_MIN);

            if (settings.showKey) {
                ImGui::InputText("##apiKey", &settings.apiKey,
                    ImGuiInputTextFlags_None);
            } else {
                std::string masked(settings.apiKey.size(), '*');
                ImGui::InputText("##apiKey", &masked,
                    ImGuiInputTextFlags_ReadOnly);
            }

            ImGui::SameLine();
            if (ImGui::Button(settings.showKey ? "Hide" : "Show")) {
                settings.showKey = !settings.showKey;
            }

            // Enable/Disable
            ImGui::Checkbox("Enable Provider", &settings.enabled);

            // Default Model
            ImGui::Text("Default Model");
            ImGui::InputText("##defaultModel", &settings.defaultModel);

            // Test Connection / Fetch Models
            if (ImGui::Button("Fetch Models")) {
                AIService::getInstance().fetchModels(provider,
                    [this, provider](std::vector<AIService::Model> models) {
                        App::runOnMain([this, provider, models]() {
                            if (provider == currentProvider_) {
                                availableModels_ = models;
                                modelsLoaded_ = true;
                                if (!models.empty()) {
                                    currentModel_ = models[0].id;
                                }
                            }
                        });
                    });
            }

            ImGui::SameLine();

            // Save button
            if (ImGui::Button("Save Settings")) {
                AIService::ProviderConfig newConfig;
                newConfig.provider = provider;
                newConfig.apiKey = settings.apiKey;
                newConfig.enabled = settings.enabled;
                newConfig.defaultModel = settings.defaultModel;
                newConfig.baseUrl = config.baseUrl;
                AIService::getInstance().setProviderConfig(provider, newConfig);

                // Update current provider if needed
                if (settings.enabled && currentModel_.empty()) {
                    currentModel_ = settings.defaultModel;
                }

                chatHistory_.push_back({"system",
                    "Settings saved for " + name, 0});
            }

            ImGui::PopItemWidth();
            ImGui::Unindent(10.0f);
            ImGui::TreePop();
        }

        ImGui::PopID();
        ImGui::Spacing();
    }

    // Show connected providers summary
    ImGui::Separator();
    ImGui::Text("Connected Providers:");
    auto enabled = AIService::getInstance().getEnabledProviders();
    if (enabled.empty()) {
        ImGui::TextDisabled("  None - enable at least one provider to use AI features");
    } else {
        for (auto p : enabled) {
            auto& config = AIService::getInstance().getProviderConfig(p);
            ImGui::BulletText("%s (%d models)", AIService::getProviderName(p).c_str(),
                (int)AIService::getInstance().getModels(p).size());
        }
    }

    // Error log
    if (!errorLog_.empty()) {
        ImGui::Separator();
        ImGui::TextColored(COLOR_ERROR, "Last Error:");
        ImGui::TextWrapped("%s", errorLog_.c_str());
        if (ImGui::Button("Clear Error Log")) {
            errorLog_.clear();
        }
    }

    ImGui::EndChild();
}

void AIWidget::renderCircuitContext() {
    if (!showContextPanel_) return;

    ImGui::BeginChild("ContextPanel", ImVec2(0, 80), true);

    auto& backend = getMainWindow().getEnvironment().getBackend();
    auto& circuitManager = backend.getCircuitManager();

    ImGui::Text("Current Circuit Context:");
    for (const auto& [id, circuit] : circuitManager.getCircuits()) {
        if (!circuit.closed()) {
            const auto& container = circuit.getBlockContainer();
            ImGui::Text("  %s - %d blocks, %d cells",
                circuit.getCircuitName().c_str(),
                container.getBlockCount(),
                container.getCellCount());
        }
    }

    if (pendingDiffs_.empty()) {
        ImGui::TextDisabled("  No pending changes");
    } else {
        ImGui::Text("  %d pending change(s):", (int)pendingDiffs_.size());
        for (const auto& diff : pendingDiffs_) {
            ImGui::Text("    %s: %s", diff.type.c_str(), diff.description.c_str());
        }
    }

    ImGui::EndChild();
}

void AIWidget::renderDiffPanel() {
    // TODO: Implement diff panel showing changes before applying
}

void AIWidget::update() {
    // Check if loading state changed
    bool loading = getGUIValue<bool>("aiLoading");
    if (loading != isLoading_) {
        // State updated from background thread
    }
}

nlohmann::json AIWidget::dumpState() const {
    nlohmann::json json;
    json["type"] = "AIWidget";
    json["chatHistory"] = chatHistory_.size();
    json["currentProvider"] = AIService::getProviderName(currentProvider_);
    json["currentModel"] = currentModel_;
    return json;
}
