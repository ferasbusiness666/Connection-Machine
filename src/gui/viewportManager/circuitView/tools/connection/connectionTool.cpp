#include "connectionTool.h"

#include "tensorConnectTool.h"
#include "singleConnectTool.h"

ConnectionTool::ConnectionTool(const Environment& environment) : CircuitTool(environment) {
	activeConnectionTool = std::make_shared<SingleConnectTool>(environment);
}

void ConnectionTool::activate() {
	if (activeConnectionTool) {
		toolStackInterface->pushTool(activeConnectionTool);
	}
}

void ConnectionTool::setMode(std::string toolMode) {
	if (mode != toolMode) {
		SharedCircuitTool newActiveConnectionTool;
		if (toolMode == "Single") {
			newActiveConnectionTool = std::make_shared<SingleConnectTool>(environment);
		} else if (toolMode == "Tensor") {
			newActiveConnectionTool = std::make_shared<TensorConnectTool>(environment);
		} else {
			logError("Tool mode \"{}\" could not be found", "", toolMode);
			return;
		}
		activeConnectionTool = newActiveConnectionTool;
		toolStackInterface->popAbove(this);
	}
}
