#ifndef setEvaluatorStateCommand_h
#define setEvaluatorStateCommand_h

#include "../command.h"

class SetEvaluatorStateCommand : public Command {
public:
	SetEvaluatorStateCommand() : Command("set_evaluator_state") {}

	void run(const std::vector<std::string>& args, Environment& environment) override final;
	const std::string getHelpString() const override final { return "Sets the state of the specified evaluator ID."; }
};

#endif /* setEvaluatorStateCommand_h */
