#include "network.h"
#include "gui/mainWindow/popUps/popUpManager.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

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
			if (Network::checkConnectedToInternet()) {
				popUpManager.addOptionsPopUp("Feedback sending failed.", { {"OK", []() {}} }, true);
			} else {
				popUpManager.addOptionsPopUp("No internet connection.", { {"OK", []() {}} }, true);
			}
		}
	});
	t.detach();
	logInfo("Feedback thread dispatched", "Network");
}
