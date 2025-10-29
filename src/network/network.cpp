#include "network.h"
#include "gui/mainWindow/popUps/popUpManager.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

std::optional<Network> Network::singletonInstance;

Network& Network::get() {
	if (!singletonInstance.has_value()) {
		singletonInstance.emplace();
	}
	return singletonInstance.value();
}
void Network::kill() {
	singletonInstance.reset();
}

Network::Network() : kvStore(KVStore::getStore("network")) {
}

Network::~Network() {
}

bool Network::checkConnectedToInternet() {
	httplib::SSLClient cli("www.gstatic.com");
	cli.set_connection_timeout(5, 0);
	cli.set_read_timeout(10, 0);
	cli.enable_server_certificate_verification(true);
	auto res = cli.Get("/generate_204");
	if (res && res->status == 204) {
		return true;
	}
	return false;
}

void Network::sendFeedback(PopUpManager& popUpManager, const std::string& title, const std::string& description, const std::vector<std::string>& attachments) {
	std::thread t([&popUpManager, title, description, attachments]() {
		httplib::SSLClient cli("api.connection-machine.com");
		cli.set_read_timeout(10, 0);
		cli.set_connection_timeout(5, 0);
		cli.enable_server_certificate_verification(true);

		const nlohmann::json jsonData = {
			{"title", title},
			{"description", description}
		};

		std::string body = jsonData.dump();

		logInfo("Sending feedback: {}", "Network", body);

		httplib::Result res = cli.Post("/api/reports", body, "application/json");
		if (res) {
			logInfo("Feedback response content: {}", "Network", res->body);
		}
		if (res && res->status / 100 == 2) {
			popUpManager.addOptionsPopUp("Feedback sent successfully!", {{"OK", []() {}}}, true);
		} else {
			if (Network::get().checkConnectedToInternet()) {
				popUpManager.addOptionsPopUp("Feedback sending failed.", { {"OK", []() {}} }, true);
			} else {
				popUpManager.addOptionsPopUp("No internet connection.", { {"OK", []() {}} }, true);
			}
		}
	});
	t.detach();
	logInfo("Feedback thread dispatched", "Network");
}

void Network::checkForUpdates(PopUpManager& popUpManager) {
	std::thread t([this, &popUpManager]() {
		Version currentVersion = parseVersionString(PROJECT_VERSION);
		logInfo("Checking for updates, current version: {}", "Network", currentVersion.toString());
		std::string ignoredVersionStr = kvStore->get<KVType::STRING>("ignored_version").value_or("0.0.0");
		Version ignoredVersion = parseVersionString(ignoredVersionStr);
		logInfo("Minimum version to notify greater than: {}", "Network", ignoredVersion.toString());
		httplib::SSLClient cli("api.github.com");
		cli.set_read_timeout(10, 0);
		cli.set_connection_timeout(5, 0);
		cli.enable_server_certificate_verification(true);
		httplib::Headers headers = {
			{"Accept", "application/vnd.github+json"},
			{"X-GitHub-Api-Version", "2022-11-28"},
			{"User-Agent", "Connection-Machine-App"}
		};
		httplib::Result res = cli.Get("/repos/Martian-Technologies/Connection-Machine/releases?per_page=1", headers);
		if (res) {
			// logInfo("Update check response content: {}", "Network", res->body);
			if (res->status / 100 == 2) {
				try {
					auto jsonResponse = nlohmann::json::parse(res->body);
					if (!jsonResponse.is_array() || jsonResponse.size() == 0) {
						logError("Unexpected update check response format", "Network");
						return;
					}
					auto latestRelease = jsonResponse[0];
					std::string latestVersionStr = latestRelease["tag_name"].get<std::string>();
					Version latestVersion = parseVersionString(latestVersionStr);
					if (latestVersion <= ignoredVersion) {
						logInfo("Latest version {} is less than or equal to ignored version {}, not notifying", "Network", latestVersion.toString(), ignoredVersion.toString());
						return;
					}
					logInfo("Latest version from update check: {}", "Network", latestVersion.toString());
					if (latestVersion <= currentVersion) {
						logInfo("No update available", "Network");
						return;
					}
					std::string body = latestRelease["body"].get<std::string>();
					std::string message = "A new version of Connection Machine is available!";
					std::string smallMessage = "Current version: " + currentVersion.toString() + "\n";
					smallMessage += "Latest version: " + latestVersion.toString() + "\n";
					smallMessage += "Changes:\n" + body;
					popUpManager.addOptionsPopUp(message, smallMessage, { {"OK", []() {}}, {"Ignore Version", [this, latestVersion]() {
						kvStore->set<KVType::STRING>("ignored_version", latestVersion.toString());
					}} }, true);
				} catch (const nlohmann::json::parse_error& e) {
					logError("Failed to parse update check response JSON: {}", "Network", e.what());
					return;
				}
			}
		}
	});
	t.detach();
	logInfo("Update check thread dispatched", "Network");
}

int Network::parseIntWithJunk(const std::string& str) {
	std::string str2 = str;
	size_t i = 0;
	while (i < str2.size()) {
		try {
			int value = std::stoi(str2);
			return value;
		} catch (const std::invalid_argument&) {
			str2 = str2.substr(0, str2.size() - 1);
		} catch (const std::out_of_range&) {
			return 0;
		}
	}
	return 0;
}

Network::Version Network::parseVersionString(const std::string& versionStr) {
	Network::Version version{0, 0, 0};
	size_t firstDot = versionStr.find('.');
	size_t secondDot = versionStr.find('.', firstDot + 1);

	if (firstDot != std::string::npos) {
		version.major = parseIntWithJunk(versionStr.substr(0, firstDot));
		if (secondDot != std::string::npos) {
			version.minor = parseIntWithJunk(versionStr.substr(firstDot + 1, secondDot - firstDot - 1));
			version.patch = parseIntWithJunk(versionStr.substr(secondDot + 1));
		} else {
			version.minor = parseIntWithJunk(versionStr.substr(firstDot + 1));
		}
	} else {
		version.major = parseIntWithJunk(versionStr);
	}

	return version;
}
