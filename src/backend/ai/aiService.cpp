#include "aiService.h"

#include <httplib.h>
#include <thread>
#include <sstream>

#include "backend/settings/settings.h"
#include "logging/logging.h"

// ================ HttpClient helper ================

class AIService::HttpClient {
public:
    std::string request(const std::string& url, const std::string& method,
                        const std::string& body,
                        const std::vector<std::pair<std::string, std::string>>& headers,
                        int& statusCode) {
        try {
            std::string scheme, host, portStr, path;
            size_t schemeEnd = url.find("://");
            if (schemeEnd == std::string::npos) return "";

            scheme = url.substr(0, schemeEnd);
            size_t hostStart = schemeEnd + 3;
            size_t pathStart = url.find('/', hostStart);

            if (pathStart == std::string::npos) {
                host = url.substr(hostStart);
                path = "/";
            } else {
                host = url.substr(hostStart, pathStart - hostStart);
                path = url.substr(pathStart);
            }

            size_t colonPos = host.find(':');
            int port = (scheme == "https") ? 443 : 80;
            if (colonPos != std::string::npos) {
                portStr = host.substr(colonPos + 1);
                port = std::stoi(portStr);
                host = host.substr(0, colonPos);
            }

            bool isHttps = (scheme == "https");

            httplib::Client client(host, port);
            if (isHttps) {
                client.enable_server_certificate_verification(false);
            }
            client.set_connection_timeout(std::chrono::seconds(30));
            client.set_read_timeout(std::chrono::seconds(120));
            client.set_write_timeout(std::chrono::seconds(30));

            httplib::Headers httplibHeaders;
            for (const auto& [key, value] : headers) {
                httplibHeaders.emplace(key, value);
            }

            std::shared_ptr<httplib::Response> res;
            if (method == "GET") {
                res = client.Get(path.c_str(), httplibHeaders);
            } else {
                res = client.Post(path.c_str(), httplibHeaders, body, "application/json");
            }

            if (res) {
                statusCode = res->status;
                return res->body;
            }

            statusCode = 0;
            return "";
        } catch (const std::exception& e) {
            logError(std::string("HTTP request failed: ") + e.what(), "AIService");
            statusCode = 0;
            return "";
        }
    }
};

// ================ AIService implementation ================

AIService& AIService::getInstance() {
    static AIService instance;
    return instance;
}

AIService::AIService() : httpClient_(std::make_unique<HttpClient>()) {
    initDefaultProviders();
    loadSettings();
}

AIService::~AIService() = default;

std::string AIService::getProviderName(Provider p) {
    switch (p) {
        case Provider::GROQ: return "Groq";
        case Provider::OPENROUTER: return "OpenRouter";
        case Provider::GOOGLE_AI_STUDIO: return "Google AI Studio";
        default: return "Unknown";
    }
}

std::string AIService::getProviderKey(Provider p) {
    switch (p) {
        case Provider::GROQ: return "groq";
        case Provider::OPENROUTER: return "openrouter";
        case Provider::GOOGLE_AI_STUDIO: return "googleAi";
        default: return "unknown";
    }
}

void AIService::initDefaultProviders() {
    providerConfigs_[static_cast<int>(Provider::GROQ)] = {
        Provider::GROQ, "", "https://api.groq.com/openai/v1", "llama-3.3-70b-versatile", false
    };
    providerConfigs_[static_cast<int>(Provider::OPENROUTER)] = {
        Provider::OPENROUTER, "", "https://openrouter.ai/api/v1", "google/gemini-2.0-flash-001", false
    };
    providerConfigs_[static_cast<int>(Provider::GOOGLE_AI_STUDIO)] = {
        Provider::GOOGLE_AI_STUDIO, "", "https://generativelanguage.googleapis.com/v1beta", "gemini-2.0-flash", false
    };
}

void AIService::setProviderConfig(Provider provider, const ProviderConfig& config) {
    std::lock_guard lock(mux_);
    providerConfigs_[static_cast<int>(provider)] = config;
    saveSettings();
}

const AIService::ProviderConfig& AIService::getProviderConfig(Provider provider) const {
    std::lock_guard lock(mux_);
    return providerConfigs_[static_cast<int>(provider)];
}

std::vector<AIService::Provider> AIService::getEnabledProviders() const {
    std::lock_guard lock(mux_);
    std::vector<Provider> enabled;
    for (const auto& config : providerConfigs_) {
        if (config.enabled && !config.apiKey.empty()) {
            enabled.push_back(config.provider);
        }
    }
    return enabled;
}

void AIService::fetchModels(Provider provider, std::function<void(std::vector<Model>)> callback) {
    std::thread([this, provider, callback]() {
        std::string url = getProviderModelsEndpoint(provider);
        if (url.empty()) {
            if (callback) callback({});
            return;
        }

        ProviderConfig config;
        {
            std::lock_guard lock(mux_);
            config = providerConfigs_[static_cast<int>(provider)];
        }

        std::vector<std::pair<std::string, std::string>> headers;

        if (provider != Provider::GOOGLE_AI_STUDIO) {
            headers.emplace_back("Authorization", "Bearer " + config.apiKey);
        } else {
            url += "?key=" + config.apiKey;
        }

        int statusCode = 0;
        std::string responseBody = httpClient_->request(url, "GET", "", headers, statusCode);

        std::vector<Model> models;
        if (!responseBody.empty() && statusCode >= 200 && statusCode < 300) {
            models = parseModelsResponse(provider, responseBody);
        }

        {
            std::lock_guard lock(mux_);
            providerModels_[static_cast<int>(provider)] = models;
            allModels_.clear();
            for (const auto& arr : providerModels_) {
                for (const auto& m : arr) {
                    allModels_.push_back(m);
                }
            }
        }

        if (callback) callback(models);
    }).detach();
}

const std::vector<AIService::Model>& AIService::getModels(Provider provider) const {
    std::lock_guard lock(mux_);
    return providerModels_[static_cast<int>(provider)];
}

const std::vector<AIService::Model>& AIService::getAllModels() const {
    std::lock_guard lock(mux_);
    return allModels_;
}

void AIService::sendChatRequest(const ChatRequest& request, Provider provider,
                                std::function<void(ChatResponse)> callback) {
    std::thread([this, request, provider, callback]() {
        ProviderConfig config;
        {
            std::lock_guard lock(mux_);
            config = providerConfigs_[static_cast<int>(provider)];
        }

        std::string url;
        std::vector<std::pair<std::string, std::string>> headers;
        std::string body;

        if (provider == Provider::GOOGLE_AI_STUDIO) {
            url = "https://generativelanguage.googleapis.com/v1beta/models/" +
                  request.model + ":generateContent?key=" + config.apiKey;
            nlohmann::json j;
            nlohmann::json contents = nlohmann::json::array();
            for (const auto& msg : request.messages) {
                nlohmann::json part;
                part["text"] = msg.content;
                nlohmann::json content;
                std::string role = (msg.role == "assistant") ? "model" : "user";
                content["role"] = role;
                content["parts"] = nlohmann::json::array({part});
                contents.push_back(content);
            }
            j["contents"] = contents;
            nlohmann::json genConfig;
            genConfig["temperature"] = request.temperature;
            genConfig["maxOutputTokens"] = request.maxTokens;
            j["generationConfig"] = genConfig;
            body = j.dump();
        } else {
            url = "https://" + (provider == Provider::GROQ ?
                 "api.groq.com/openai/v1/chat/completions" :
                 "openrouter.ai/api/v1/chat/completions");
            headers.emplace_back("Authorization", "Bearer " + config.apiKey);
            headers.emplace_back("Content-Type", "application/json");
            if (provider == Provider::OPENROUTER) {
                headers.emplace_back("HTTP-Referer", "https://github.com/Connection-Machine");
                headers.emplace_back("X-Title", "Connection Machine");
            }
            nlohmann::json j;
            j["model"] = request.model;
            j["temperature"] = request.temperature;
            j["max_tokens"] = request.maxTokens;
            nlohmann::json messages = nlohmann::json::array();
            for (const auto& msg : request.messages) {
                nlohmann::json m;
                m["role"] = msg.role;
                m["content"] = msg.content;
                messages.push_back(m);
            }
            j["messages"] = messages;
            body = j.dump();
        }

        int statusCode = 0;
        std::string responseBody = httpClient_->request(url, "POST", body, headers, statusCode);

        ChatResponse response;
        if (responseBody.empty()) {
            response.success = false;
            response.error = "No response from server (network error or timeout)";
        } else if (statusCode >= 200 && statusCode < 300) {
            response = parseChatResponse(provider, responseBody);
        } else {
            response.success = false;
            response.error = "HTTP " + std::to_string(statusCode) + ": " + responseBody;
        }

        logInteraction(getProviderName(provider), request.model, request.messages, response);

        if (callback) callback(response);
    }).detach();
}

std::string AIService::getProviderBaseUrl(Provider provider) const {
    switch (provider) {
        case Provider::GROQ: return "https://api.groq.com/openai/v1";
        case Provider::OPENROUTER: return "https://openrouter.ai/api/v1";
        case Provider::GOOGLE_AI_STUDIO: return "https://generativelanguage.googleapis.com/v1beta";
        default: return "";
    }
}

std::string AIService::getProviderModelsEndpoint(Provider provider) const {
    switch (provider) {
        case Provider::GROQ: return "https://api.groq.com/openai/v1/models";
        case Provider::OPENROUTER: return "https://openrouter.ai/api/v1/models";
        case Provider::GOOGLE_AI_STUDIO: return "https://generativelanguage.googleapis.com/v1beta/models";
        default: return "";
    }
}

std::string AIService::getProviderChatEndpoint(Provider provider) const {
    return getProviderBaseUrl(provider);
}

AIService::ChatResponse AIService::parseChatResponse(Provider provider, const std::string& responseBody) const {
    ChatResponse response;

    try {
        nlohmann::json json = nlohmann::json::parse(responseBody);

        switch (provider) {
            case Provider::GROQ:
            case Provider::OPENROUTER: {
                const auto& choices = json["choices"];
                if (!choices.is_null() && choices.is_array() && choices.size() > 0) {
                    response.content = choices[0]["message"]["content"].get<std::string>();
                }
                if (!json["usage"].is_null()) {
                    response.promptTokens = json["usage"]["prompt_tokens"].get<int>();
                    response.completionTokens = json["usage"]["completion_tokens"].get<int>();
                }
                response.success = !response.content.empty();
                break;
            }
            case Provider::GOOGLE_AI_STUDIO: {
                const auto& candidates = json["candidates"];
                if (!candidates.is_null() && candidates.is_array() && candidates.size() > 0) {
                    const auto& parts = candidates[0]["content"]["parts"];
                    if (!parts.is_null() && parts.is_array() && parts.size() > 0) {
                        response.content = parts[0]["text"].get<std::string>();
                    }
                }
                if (!json["usageMetadata"].is_null()) {
                    response.promptTokens = json["usageMetadata"]["promptTokenCount"].get<int>();
                    response.completionTokens = json["usageMetadata"]["candidatesTokenCount"].get<int>();
                }
                response.success = !response.content.empty();
                break;
            }
        }

        if (!response.success && json.contains("error")) {
            response.error = json["error"].dump();
        }
    } catch (const std::exception& e) {
        response.success = false;
        response.error = std::string("Parse error: ") + e.what();
    }

    return response;
}

std::vector<AIService::Model> AIService::parseModelsResponse(Provider provider, const std::string& responseBody) const {
    std::vector<Model> models;

    try {
        nlohmann::json json = nlohmann::json::parse(responseBody);

        switch (provider) {
            case Provider::GROQ:
            case Provider::OPENROUTER: {
                const auto& data = json["data"];
                if (!data.is_null() && data.is_array()) {
                    for (const auto& item : data) {
                        Model model;
                        model.id = item["id"].get<std::string>();
                        model.name = item.contains("name") ? item["name"].get<std::string>() : model.id;
                        model.provider = provider;
                        models.push_back(model);
                    }
                }
                break;
            }
            case Provider::GOOGLE_AI_STUDIO: {
                const auto& data = json["models"];
                if (!data.is_null() && data.is_array()) {
                    for (const auto& item : data) {
                        if (item.contains("supportedGenerationMethods")) {
                            bool supportsChat = false;
                            for (const auto& method : item["supportedGenerationMethods"]) {
                                std::string m = method.get<std::string>();
                                if (m.find("generateContent") != std::string::npos) {
                                    supportsChat = true;
                                    break;
                                }
                            }
                            if (!supportsChat) continue;
                        }
                        Model model;
                        std::string fullId = item["name"].get<std::string>();
                        if (fullId.find("models/") == 0) {
                            model.id = fullId.substr(7);
                        } else {
                            model.id = fullId;
                        }
                        model.name = item.contains("displayName") ? item["displayName"].get<std::string>() : model.id;
                        model.provider = provider;
                        models.push_back(model);
                    }
                }
                break;
            }
        }
    } catch (const std::exception& e) {
        logError(std::string("Failed to parse models response: ") + e.what(), "AIService");
    }

    return models;
}

void AIService::logInteraction(const std::string& providerStr, const std::string& model,
                               const std::vector<Message>& messages, const ChatResponse& response) {
    std::string status = response.success ? "Success" : "Failed";
    std::string tokenInfo = "Tokens: " + std::to_string(response.promptTokens + response.completionTokens);
    logInfo("AI Interaction | Provider: " + providerStr + " | Model: " + model +
            " | " + status + " | " + tokenInfo, "AIService");

    if (!response.success && !response.error.empty()) {
        logError("AI Interaction Error | Provider: " + providerStr + " | Model: " + model +
                 " | Error: " + response.error, "AIService");
    }
}

void AIService::loadSettings() {
    auto& settings = Settings::getSettingsMap();

    auto registerIfMissing = [&](const std::string& key, SettingType type) {
        if (!settings.hasKey(key)) {
            switch (type) {
                case SettingType::STRING:
                    settings.registerSetting<SettingType::STRING>(key, "");
                    break;
                case SettingType::BOOL:
                    settings.registerSetting<SettingType::BOOL>(key, false);
                    break;
                default:
                    break;
            }
        }
    };

    for (int i = 0; i < PROVIDER_COUNT; i++) {
        auto provider = static_cast<Provider>(i);
        std::string key = getProviderKey(provider);

        registerIfMissing("ai/" + key + "/apiKey", SettingType::STRING);
        registerIfMissing("ai/" + key + "/enabled", SettingType::BOOL);
        registerIfMissing("ai/" + key + "/defaultModel", SettingType::STRING);
        registerIfMissing("ai/" + key + "/baseUrl", SettingType::STRING);

        auto* apiKey = Settings::get<SettingType::STRING>("ai/" + key + "/apiKey");
        if (apiKey) providerConfigs_[i].apiKey = *apiKey;

        auto* enabled = Settings::get<SettingType::BOOL>("ai/" + key + "/enabled");
        if (enabled) providerConfigs_[i].enabled = *enabled;

        auto* defaultModel = Settings::get<SettingType::STRING>("ai/" + key + "/defaultModel");
        if (defaultModel && !defaultModel->empty()) providerConfigs_[i].defaultModel = *defaultModel;
    }
}

void AIService::saveSettings() {
    for (int i = 0; i < PROVIDER_COUNT; i++) {
        auto provider = static_cast<Provider>(i);
        std::string key = getProviderKey(provider);

        Settings::set<SettingType::STRING>("ai/" + key + "/apiKey", providerConfigs_[i].apiKey);
        Settings::set<SettingType::BOOL>("ai/" + key + "/enabled", providerConfigs_[i].enabled);
        Settings::set<SettingType::STRING>("ai/" + key + "/defaultModel", providerConfigs_[i].defaultModel);
    }
}
