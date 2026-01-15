#ifndef createSimulatorCommand_h
#define createSimulatorCommand_h

#include "../command.h"

class CreateSimulatorCommand : public Command {
public:
	CreateSimulatorCommand() : Command("create_evaluator") {}

	void run(const std::vector<std::string>& args, Environment& environment) override final;
	const std::string getHelpString() const override final { return "Creates an evaluator for the specified circuit."; }
};

#endif /* createSimulatorCommand_h */