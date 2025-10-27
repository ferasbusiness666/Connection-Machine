#ifndef network_h
#define network_h

class PopUpManager;

class Network {
public:
    static bool checkConnectedToInternet();
    static void sendFeedback(PopUpManager& popUpManager, const std::string& title, const std::string& description, const std::vector<std::string>& attachments);

private:
};

#endif /* network_h */