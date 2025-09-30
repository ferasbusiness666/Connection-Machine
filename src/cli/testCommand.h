#ifndef testCommand_h
#define testCommand_h

#include "command.h"

class testCommand : public Command {
public:
    const std::string& getName() { return "testCommand"}
};

#endif