#ifndef network_h
#define network_h

#include "computerAPI/kvStore.h"

class PopUpManager;

class Network {
public:
	static Network& get();
	static void kill();

	bool checkConnectedToInternet();
	void sendFeedback(PopUpManager& popUpManager, const std::string& title, const std::string& description, const std::vector<std::string>& attachments);
	void checkForUpdates(PopUpManager& popUpManager);

	// do not call
	Network();
	~Network();

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
	static std::optional<Network> singletonInstance;

	std::shared_ptr<KVStore> kvStore;
};

#endif /* network_h */
