#include "evaluatorInternal.h"

#include "backend/blockData/blockDataManager.h"
// #include "backend/circuit/circuitBlockDataManager.h"

EvaluatorInternal::EvaluatorInternal(const BlockDataManager& blockDataManager, const CircuitBlockDataManager& circuitBlockDataManager) :
	evalGateIdProvider(1), blockDataManager(blockDataManager), circuitBlockDataManager(circuitBlockDataManager),
	layerRunner(blockDataManager) { }

void EvaluatorInternal::startEdit() {
	layerRunner.getInputLayer().resetEdits();
}

void EvaluatorInternal::endEdit() {
	layerRunner.runAll();
}

void EvaluatorInternal::addBlock(Position position, Orientation orientation, BlockType blockType) {
	eval_gate_id evalId = evalGateIdProvider.getNewId();
	positionRemapping.try_emplace(position, evalId, orientation);
	positionReverseRemapping.try_emplace(evalId, position, orientation);
	layerRunner.getInputLayer().addGate(evalId, getEvalGateType(blockType));
}

void EvaluatorInternal::removeBlock(Position position, Orientation orientation, BlockType blockType) {
	auto remappingIter = positionRemapping.find(position);
	if (remappingIter == positionRemapping.end()) {
		logError("Could not find placed gate at {} when removing gate.", "evaluatorInternal", position);
		return;
	}
	layerRunner.getInputLayer().removeGate(remappingIter->second.first);
	positionRemapping.erase(remappingIter);
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
	positionReverseRemapping.at(evalId) = {newPosition, newOrientation};
}

void EvaluatorInternal::createConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition) {
	auto outputRemappingIter = positionRemapping.find(outputBlockPosition);
	if (outputRemappingIter == positionRemapping.end()) return;
	const EvalGate* outputGate = layerRunner.getInputLayer().getGate(outputRemappingIter->second.first);
	assert(outputGate);
	const BlockData* outputBlockData = blockDataManager.getBlockData(getBlockType(outputGate->type));
	std::optional<connection_end_id_t> outputConnectionEndId = outputBlockData->getOutputOrBidirectionalConnectionId(outputPosition - outputBlockPosition, outputRemappingIter->second.second);
	assert(outputConnectionEndId);

	auto inputRemappingIter = positionRemapping.find(inputBlockPosition);
	if (inputRemappingIter == positionRemapping.end()) return;
	const EvalGate* inputGate = layerRunner.getInputLayer().getGate(inputRemappingIter->second.first);
	assert(inputGate);
	const BlockData* inputBlockData = blockDataManager.getBlockData(getBlockType(inputGate->type));
	std::optional<connection_end_id_t> inputConnectionEndId = inputBlockData->getInputOrBidirectionalConnectionId(inputPosition - inputBlockPosition, inputRemappingIter->second.second);
	assert(inputConnectionEndId);

	layerRunner.getInputLayer().addConnection(EvalConnection(outputGate->gateId, *outputConnectionEndId, inputGate->gateId, *inputConnectionEndId));
}

void EvaluatorInternal::removeConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition) {
	auto outputRemappingIter = positionRemapping.find(outputBlockPosition);
	if (outputRemappingIter == positionRemapping.end()) return;
	const EvalGate* outputGate = layerRunner.getInputLayer().getGate(outputRemappingIter->second.first);
	assert(outputGate);
	const BlockData* outputBlockData = blockDataManager.getBlockData(getBlockType(outputGate->type));
	std::optional<connection_end_id_t> outputConnectionEndId = outputBlockData->getOutputOrBidirectionalConnectionId(outputPosition - outputBlockPosition, outputRemappingIter->second.second);
	assert(outputConnectionEndId);

	auto inputRemappingIter = positionRemapping.find(inputBlockPosition);
	if (inputRemappingIter == positionRemapping.end()) return;
	const EvalGate* inputGate = layerRunner.getInputLayer().getGate(inputRemappingIter->second.first);
	assert(inputGate);
	const BlockData* inputBlockData = blockDataManager.getBlockData(getBlockType(inputGate->type));
	std::optional<connection_end_id_t> inputConnectionEndId = inputBlockData->getInputOrBidirectionalConnectionId(inputPosition - inputBlockPosition, inputRemappingIter->second.second);
	assert(inputConnectionEndId);

	layerRunner.getInputLayer().removeConnection(EvalConnection(outputGate->gateId, *outputConnectionEndId, inputGate->gateId, *inputConnectionEndId));
}
