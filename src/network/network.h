#ifndef network_h
#define network_h

#include <httplib.h>
#include "logging/logging.h"

class Network {
public:
    static Network& get();
    static void kill();
    bool checkConnectedToInternet();

    // do not call
    Network();
    ~Network();

private:
};

#endif /* network_h */