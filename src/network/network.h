#ifndef network_h
#define network_h

class PopUpManager;

class Network {
public:
    static bool checkConnectedToInternet();
    static void sendFeedback(PopUpManager& popUpManager, const std::string& title, const std::string& description, const std::vector<std::string>& attachments);
    static void checkForUpdates(PopUpManager& popUpManager);

private:
    struct Version {
        int major;
        int minor;
        int patch;
        std::string toString() const {
            return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
        }
        auto operator<=>(const Version&) const = default;
    };
    static int parseIntWithJunk(const std::string& str);
    static Version parseVersionString(const std::string& versionStr);
};

#endif /* network_h */