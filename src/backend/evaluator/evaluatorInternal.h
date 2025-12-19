#ifndef evaluatorInternal_h
#define evaluatorInternal_h

#include "evalDefs.h"

#include "backend/evaluator/util/evalLayerState.h"

class EvaluatorInternal {
public:
	EvaluatorInternal() : evalGateIdProvider(1) { }
	void startEdit();
	void endEdit();
	void addBlock(Position position, Orientation orientation, BlockType blockType);
	void removeBlock(Position position, Orientation orientation, BlockType blockType);
	void moveBlock(Position curPosition, Orientation curOrientation, Position newPosition, Orientation newOrientation);
	void removeConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition);
	void createConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition);

private:
	IdProvider<eval_gate_id> evalGateIdProvider;
	std::unordered_map<Position, eval_gate_id> positionRemapping;
	EvalLayerState before;
	EvalLayerState after;
};

#endif /* evaluatorInternal_h */
