#ifndef aiService_h
#define aiService_h

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <mutex>
#include <chrono>
#include <memory>
#include <array>
#include <nlohmann/json.hpp>

class AIService {
public:
    enum class Provider {
        GROQ,
        OPENROUTER,
        GOOGLE_AI_STUDIO,
        NUM_PROVIDERS
    };

    static constexpr int PROVIDER_COUNT = static_cast<int>(Provider::NUM_PROVIDERS);

    struct Model {
        std::string id;
        std::string name;
        Provider provider;
    };

    struct Message {
        std::string role;
        std::string content;
    };

    struct ChatRequest {
        std::vector<Message> messages;
        std::string model;
        double temperature = 0.7;
        int maxTokens = 4096;
    };

    struct ChatResponse {
        bool success = false;
        std::string content;
        std::string error;
        int promptTokens = 0;
        int completionTokens = 0;
    };

    struct ProviderConfig {
        Provider provider;
        std::string apiKey;
        std::string baseUrl;
        std::string defaultModel;
        bool enabled = false;
    };

    static AIService& getInstance();

    void setProviderConfig(Provider provider, const ProviderConfig& config);
    const ProviderConfig& getProviderConfig(Provider provider) const;
    std::vector<Provider> getEnabledProviders() const;
    
    void fetchModels(Provider provider, std::function<void(std::vector<Model>)> callback);
    const std::vector<Model>& getModels(Provider provider) const;
    const std::vector<Model>& getAllModels() const;
    
    void sendChatRequest(const ChatRequest& request, Provider provider, std::function<void(ChatResponse)> callback);
    
    void loadSettings();
    void saveSettings();
    
    void logInteraction(const std::string& provider, const std::string& model, 
                       const std::vector<Message>& messages, const ChatResponse& response);

    static std::string getProviderName(Provider p);
    static std::string getProviderKey(Provider p);

private:
    AIService();
    ~AIService();
    AIService(const AIService&) = delete;
    AIService& operator=(const AIService&) = delete;

    void initDefaultProviders();
    std::string getProviderBaseUrl(Provider provider) const;
    std::string getProviderModelsEndpoint(Provider provider) const;
    std::string getProviderChatEndpoint(Provider provider) const;
    
    nlohmann::json buildChatRequestJson(const ChatRequest& request, Provider provider) const;
    ChatResponse parseChatResponse(Provider provider, const std::string& responseBody) const;
    std::vector<Model> parseModelsResponse(Provider provider, const std::string& responseBody) const;

    mutable std::mutex mux_;
    std::array<ProviderConfig, PROVIDER_COUNT> providerConfigs_;
    std::array<std::vector<Model>, PROVIDER_COUNT> providerModels_;
    std::vector<Model> allModels_;
    
    class HttpClient;
    std::unique_ptr<HttpClient> httpClient_;
};

#endif // aiService_h