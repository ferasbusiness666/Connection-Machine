#ifndef simulatorTickStepCommand_h
#define simulatorTickStepCommand_h

#include "../command.h"

class SimulatorTickStepCommand : public Command {
public:
	SimulatorTickStepCommand() : Command("simulator_tick_step") {}

	void run(const std::vector<std::string>& args, Environment& environment) override final;
	const std::string getHelpString() const override final { return "Steps an simulator forwards by a specified number of ticks, default of 1."; }
};

#endif /* simulatorTickStepCommand_h */