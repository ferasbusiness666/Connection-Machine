#include "evaluatorInternal.h"

#include "backend/blockData/blockDataManager.h"
// #include "backend/circuit/circuitBlockDataManager.h"

void EvaluatorInternal::startEdit() {
	before.resetEdits();
}

void EvaluatorInternal::endEdit() {
	layerRunner.runAll();
}

void EvaluatorInternal::addBlock(Position position, Orientation orientation, BlockType blockType) {
	eval_gate_id evalId = evalGateIdProvider.getNewId();
	positionRemapping.try_emplace(position, evalId, orientation);
	before.addGate(evalId, getEvalGateType(blockType));
}

void EvaluatorInternal::removeBlock(Position position, Orientation orientation, BlockType blockType) {
	auto remappingIter = positionRemapping.find(position);
	if (remappingIter == positionRemapping.end()) {
		logError("Could not find placed gate at {} when removing gate.", "evaluatorInternal", position);
		return;
	}
	before.removeGate(remappingIter->second.first);
}

void EvaluatorInternal::moveBlock(Position curPosition, Orientation curOrientation, Position newPosition, Orientation newOrientation) {
	auto remappingIter = positionRemapping.find(curPosition);
	if (remappingIter == positionRemapping.end()) {
		logError("Could not find placed gate at {} when moving gate.", "evaluatorInternal", curPosition);
		return;
	}
	eval_gate_id evalId = remappingIter->second.first;
	positionRemapping.erase(remappingIter);
	positionRemapping.try_emplace(newPosition, evalId, newOrientation);
}

void EvaluatorInternal::createConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition) {
	auto outputRemappingIter = positionRemapping.find(outputBlockPosition);
	if (outputRemappingIter == positionRemapping.end()) return;
	const EvalGate* outputGate = before.getGate(outputRemappingIter->second.first);
	assert(outputGate);
	const BlockData* outputBlockData = blockDataManager.getBlockData(getBlockType(outputGate->type));
	std::optional<connection_end_id_t> outputConnectionEndId = outputBlockData->getOutputOrBidirectionalConnectionId(outputPosition - outputBlockPosition, outputRemappingIter->second.second);
	assert(outputConnectionEndId);

	auto inputRemappingIter = positionRemapping.find(inputBlockPosition);
	if (inputRemappingIter == positionRemapping.end()) return;
	const EvalGate* inputGate = before.getGate(inputRemappingIter->second.first);
	assert(inputGate);
	const BlockData* inputBlockData = blockDataManager.getBlockData(getBlockType(inputGate->type));
	std::optional<connection_end_id_t> inputConnectionEndId = inputBlockData->getInputOrBidirectionalConnectionId(inputPosition - inputBlockPosition, inputRemappingIter->second.second);
	assert(inputConnectionEndId);

	before.addConnection(EvalConnection(outputGate->gateId, *outputConnectionEndId, inputGate->gateId, *inputConnectionEndId));
}

void EvaluatorInternal::removeConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition) {
	auto outputRemappingIter = positionRemapping.find(outputBlockPosition);
	if (outputRemappingIter == positionRemapping.end()) return;
	const EvalGate* outputGate = before.getGate(outputRemappingIter->second.first);
	assert(outputGate);
	const BlockData* outputBlockData = blockDataManager.getBlockData(getBlockType(outputGate->type));
	std::optional<connection_end_id_t> outputConnectionEndId = outputBlockData->getOutputOrBidirectionalConnectionId(outputPosition - outputBlockPosition, outputRemappingIter->second.second);
	assert(outputConnectionEndId);

	auto inputRemappingIter = positionRemapping.find(inputBlockPosition);
	if (inputRemappingIter == positionRemapping.end()) return;
	const EvalGate* inputGate = before.getGate(inputRemappingIter->second.first);
	assert(inputGate);
	const BlockData* inputBlockData = blockDataManager.getBlockData(getBlockType(inputGate->type));
	std::optional<connection_end_id_t> inputConnectionEndId = inputBlockData->getInputOrBidirectionalConnectionId(inputPosition - inputBlockPosition, inputRemappingIter->second.second);
	assert(inputConnectionEndId);

	before.removeConnection(EvalConnection(outputGate->gateId, *outputConnectionEndId, inputGate->gateId, *inputConnectionEndId));
}
