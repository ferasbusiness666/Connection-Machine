#ifndef toolManager_h
#define toolManager_h

#include "toolStack.h"

class CircuitView;

class ToolManager {
public:
	inline ToolManager(Environment& environment, EventRegister& eventRegister, ViewportId viewportId, CircuitView& circuitView) :
		toolStacks {
			ToolStack(environment, eventRegister, viewportId, circuitView, *this),
			ToolStack(environment, eventRegister, viewportId, circuitView, *this),
			ToolStack(environment, eventRegister, viewportId, circuitView, *this)
		}, environment(environment) {
		toolStacks[activeToolStack].activate();
	}

	void selectStack(int stack);
	void setOrientation(Orientation orientation);
	int getStack() const { return activeToolStack; }
	void selectBlock(BlockType blockType);
	SharedCircuitTool selectTool(SharedCircuitTool tool);

	void setMode(const std::string& mode);

	inline void setCircuit(circuit_id_t circuitId) {
		this->circuitId = circuitId;
		for (auto& toolStack : toolStacks) toolStack.setCircuit(circuitId);
	}

private:
	int activeToolStack = 0;
	circuit_id_t circuitId = 0;
	std::array<ToolStack, 3> toolStacks;
	std::map<std::string, SharedCircuitTool> toolInstances;
	Environment& environment;
};

#endif /* toolManager_h */
