/*#ifndef resetSimulatoruatorCommand_h
#define resetSimulatoruatorCommand_h

#include "../command.h"

class ResetSimulatoruatorCommand : public Command {
public:
	ResetSimulatoruatorCommand() : Command("reset_simulator") {}

	void run(const std::vector<std::string>& args, Environment& environment) override final;
	const std::string getHelpString() const override final { return "Resets the simulator of specified ID."; }
};

#endif /* resetSimulatoruatorCommand_h */
