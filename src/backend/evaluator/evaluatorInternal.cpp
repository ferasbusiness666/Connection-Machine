#include "evaluatorInternal.h"

void EvaluatorInternal::startEdit() {
	before.resetEdits();
}

void EvaluatorInternal::endEdit() {

}

void EvaluatorInternal::addBlock(Position position, Orientation orientation, BlockType blockType) {
	eval_gate_id evalId = evalGateIdProvider.getNewId();
	positionRemapping.try_emplace(position, evalId);
	before.addGate(evalId, blockType);
}

void EvaluatorInternal::removeBlock(Position position, Orientation orientation, BlockType blockType) {
	auto iter = positionRemapping.find(position);
	if (iter == positionRemapping.end()) {
		logError("Could not find placed gate at {} when removing gate.", "evaluatorInternal", position);
		return;
	}
	before.removeGate(iter->second);
}

void EvaluatorInternal::moveBlock(Position curPosition, Orientation curOrientation, Position newPosition, Orientation newOrientation) {
	auto iter = positionRemapping.find(curPosition);
	if (iter == positionRemapping.end()) {
		logError("Could not find placed gate at {} when moving gate.", "evaluatorInternal", curPosition);
		return;
	}
	eval_gate_id evalId = iter->second;
	positionRemapping.erase(iter);
	positionRemapping.try_emplace(newPosition, evalId);
}

void EvaluatorInternal::removeConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition) {

}

void EvaluatorInternal::createConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition) {

}
