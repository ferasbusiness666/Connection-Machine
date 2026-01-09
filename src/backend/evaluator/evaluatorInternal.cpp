#include "evaluatorInternal.h"

#include "backend/circuit/circuitManager.h"
#include "backend/evaluator/layers/evalLayerState.h"

extern std::thread::id mainThreadId;

EvaluatorInternal::EvaluatorInternal(circuit_id_t circuitId, const CircuitManager& circuitManager) :
	evalGateIdProvider(1), circuitManager(circuitManager), circuitId(circuitId), layerRunner(circuitManager.getBlockDataManager()) { }

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
	const BlockData* outputBlockData = circuitManager.getBlockDataManager().getBlockData(getBlockType(outputGate->type));
	std::optional<connection_end_id_t> outputConnectionEndId = outputBlockData->getOutputOrBidirectionalConnectionId(outputPosition - outputBlockPosition, outputRemappingIter->second.second);
	assert(outputConnectionEndId);

	auto inputRemappingIter = positionRemapping.find(inputBlockPosition);
	if (inputRemappingIter == positionRemapping.end()) return;
	const EvalGate* inputGate = layerRunner.getInputLayer().getGate(inputRemappingIter->second.first);
	assert(inputGate);
	const BlockData* inputBlockData = circuitManager.getBlockDataManager().getBlockData(getBlockType(inputGate->type));
	std::optional<connection_end_id_t> inputConnectionEndId = inputBlockData->getInputOrBidirectionalConnectionId(inputPosition - inputBlockPosition, inputRemappingIter->second.second);
	assert(inputConnectionEndId);

	layerRunner.getInputLayer().addConnection(EvalConnection(outputGate->gateId, *outputConnectionEndId, inputGate->gateId, *inputConnectionEndId));
}

void EvaluatorInternal::removeConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition) {
	auto outputRemappingIter = positionRemapping.find(outputBlockPosition);
	if (outputRemappingIter == positionRemapping.end()) return;
	const EvalGate* outputGate = layerRunner.getInputLayer().getGate(outputRemappingIter->second.first);
	assert(outputGate);
	const BlockData* outputBlockData = circuitManager.getBlockDataManager().getBlockData(getBlockType(outputGate->type));
	std::optional<connection_end_id_t> outputConnectionEndId = outputBlockData->getOutputOrBidirectionalConnectionId(outputPosition - outputBlockPosition, outputRemappingIter->second.second);
	assert(outputConnectionEndId);

	auto inputRemappingIter = positionRemapping.find(inputBlockPosition);
	if (inputRemappingIter == positionRemapping.end()) return;
	const EvalGate* inputGate = layerRunner.getInputLayer().getGate(inputRemappingIter->second.first);
	assert(inputGate);
	const BlockData* inputBlockData = circuitManager.getBlockDataManager().getBlockData(getBlockType(inputGate->type));
	std::optional<connection_end_id_t> inputConnectionEndId = inputBlockData->getInputOrBidirectionalConnectionId(inputPosition - inputBlockPosition, inputRemappingIter->second.second);
	assert(inputConnectionEndId);

	layerRunner.getInputLayer().removeConnection(EvalConnection(outputGate->gateId, *outputConnectionEndId, inputGate->gateId, *inputConnectionEndId));
}

EvalConnectionPoint EvaluatorInternal::mapFromTopConnectionPointToBottomConnectionPoint(EvalConnectionPoint topConnectionPoint) const {
	return layerRunner.getMappedEvalConnectionPoint(topConnectionPoint);
}
std::optional<std::vector<EvalConnectionPoint>> EvaluatorInternal::mapFromBottomConnectionPointToTopConnectionPoint(EvalConnectionPoint bottomConnectionPoint) const {
	return layerRunner.getReversedMappedEvalConnectionPoint(bottomConnectionPoint);
}
std::optional<Address> EvaluatorInternal::mapFromTopConnectionPointToAddress(EvalConnectionPoint topConnectionPoint) const {
	if (topConnectionPoint.gateId == 0) return std::nullopt;
	auto iter = positionReverseRemapping.find(topConnectionPoint.gateId);
	if (iter == positionReverseRemapping.end()) return std::nullopt;
	return iter->second.first + circuitManager.getBlockDataManager().getConnectionVector(
		getBlockType(layerRunner.getInputLayer().getGate(topConnectionPoint.gateId)->type),
		iter->second.second,
		topConnectionPoint.connectionEndId
	).value();
}
std::optional<std::vector<Address>> EvaluatorInternal::mapFromBottomConnectionPointToAddress(EvalConnectionPoint bottomConnectionPoint) const {
	if (bottomConnectionPoint.gateId == 0) return std::nullopt;
	std::optional<std::vector<EvalConnectionPoint>> topConnectionPoints = mapFromBottomConnectionPointToTopConnectionPoint(bottomConnectionPoint);
	if (!topConnectionPoints) return std::nullopt;
	std::vector<Address> addresses;
	for (const EvalConnectionPoint& topConnectionPoint : topConnectionPoints.value()) {
		addresses.push_back(mapFromTopConnectionPointToAddress(topConnectionPoint).value());
	}
	return addresses;
}
EvalConnectionPoint EvaluatorInternal::mapFromAddressToTopConnectionPoint(const Address& address) const {
	if (address.size() != 1) return EvalConnectionPoint::null();
	assert(mainThreadId == std::this_thread::get_id());
	const Circuit* circuit = circuitManager.getCircuit(circuitId).get();
	const Block* block = circuit->getBlockContainer().getBlock(address.getPosition(0));
	if (block == nullptr) return EvalConnectionPoint::null();
	auto iter = positionRemapping.find(block->getPosition());
	if (iter == positionRemapping.end()) return EvalConnectionPoint::null();
	std::optional<connection_end_id_t> connectionEndId = block->getOutputOrBidirectionalConnectionId(address.getPosition(0));
	if (!connectionEndId) return  EvalConnectionPoint::null();
	return EvalConnectionPoint(iter->second.first, connectionEndId.value());
}
EvalConnectionPoint EvaluatorInternal::mapFromAddressToBottomConnectionPoint(const Address& address) const {
	EvalConnectionPoint topConnectionPoint = mapFromAddressToTopConnectionPoint(address);
	if (topConnectionPoint.isNull()) return EvalConnectionPoint::null();
	return mapFromTopConnectionPointToBottomConnectionPoint(topConnectionPoint);
}
