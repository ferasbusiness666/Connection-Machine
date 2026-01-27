#ifndef listSimulatorsCommand_h
#define listSimulatorsCommand_h

#include "../command.h"

class ListSimulatorsCommand : public Command {
public:
	ListSimulatorsCommand() : Command("list_simulators") {}

	void run(const std::vector<std::string>& args, Environment& environment) override final;
	const std::string getHelpString() const override final { return "Lists all open simulators."; }
};

#endif /* listSimulatorsCommand_h */
