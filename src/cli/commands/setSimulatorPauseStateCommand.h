#ifndef setSimulatorPauseStateCommand_h
#define setSimulatorPauseStateCommand_h

#include "../command.h"

class SetSimulatorPauseStateCommand : public Command {
public:
	SetSimulatorPauseStateCommand() : Command("set_eval_pause_state") {}

	void run(const std::vector<std::string>& args, Environment& environment) override final;
	const std::string getHelpString() const override final { return "Pauses or unpauses an simulator."; }
};

#endif /* setSimulatorPauseStateCommand_h */
