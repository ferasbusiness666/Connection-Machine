#ifndef evaluatorInternal_h
#define evaluatorInternal_h

#include "evalDefs.h"

#include "backend/evaluator/util/evalLayerState.h"
#include "layers/layerRunner.h"

class BlockDataManager;
class CircuitBlockDataManager;

class EvaluatorInternal {
public:
	EvaluatorInternal(const BlockDataManager& blockDataManager, const CircuitBlockDataManager& circuitBlockDataManager) :
		evalGateIdProvider(1), blockDataManager(blockDataManager), circuitBlockDataManager(circuitBlockDataManager) { }
	void startEdit();
	void endEdit();
	void addBlock(Position position, Orientation orientation, BlockType blockType);
	void removeBlock(Position position, Orientation orientation, BlockType blockType);
	void moveBlock(Position curPosition, Orientation curOrientation, Position newPosition, Orientation newOrientation);
	void removeConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition);
	void createConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition);

private:
	IdProvider<eval_gate_id> evalGateIdProvider;
	std::unordered_map<Position, std::pair<eval_gate_id, Orientation>> positionRemapping;
	EvalLayerState before;
	EvalLayerState after;
	const BlockDataManager& blockDataManager;
	const CircuitBlockDataManager& circuitBlockDataManager;
	LayerRunner layerRunner;
};

#endif /* evaluatorInternal_h */
