#include "network.h"

std::optional<Network> networkSingleton;

Network& Network::get() {
    if (!networkSingleton) {
        logInfo("Creating Network", "Network");
        networkSingleton.emplace();
    }
    return *networkSingleton;
}

void Network::kill() {
    logInfo("Killing Network", "Network");
    networkSingleton.reset();
}

Network::Network() {
    logInfo("Initializing Network", "Network");
    logInfo("Network initialized", "Network");
}

Network::~Network() {
    logInfo("Shutting down Network", "Network");
}

bool Network::checkConnectedToInternet() {
    httplib::Client cli("https://www.gstatic.com");
    auto res = cli.Get("/generate_204");
    if (res && res->status == 204) {
        return true;
    }
    return false;
}
