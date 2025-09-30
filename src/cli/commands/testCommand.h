#ifndef testCommand_h
#define testCommand_h

#include "../command.h"

class TestCommand : public Command {
public:
    void run(std::vector<std::string> commands) {
        std::cout << "This is the test command, the parameters passed are:\n" << std::flush;
        for (std::vector<std::string>::iterator args = commands.begin(); iterator != commands.end(); ++iterator) {
            std::cout << *iterator << "\n" << std::flush;
        }
    }
};

#endif