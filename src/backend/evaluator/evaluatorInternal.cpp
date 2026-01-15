#include "evaluatorInternal.h"

#include "backend/circuit/circuitManager.h"
#include "backend/evaluator/layers/evalLayerState.h"

extern std::thread::id mainThreadId;

EvaluatorInternal::EvaluatorInternal(const Circuit& circuit, const CircuitManager& circuitManager, DataUpdateEventManager::DataUpdateEventReceiver& receiver) :
	evalGateIdProvider(1), circuitManager(circuitManager), circuit(circuit), layerRunner(circuitManager) {
	receiver.linkFunction("circuitBlockDataConnectionPositionRemove", [&](const DataUpdateEventManager::EventData* event) {
		const auto* data = event->cast<std::tuple<BlockType, connection_end_id_t, Position>>();
		if (!data) return;
		BlockType blockType = std::get<0>(data->get());
		if (blockType != circuit.getBlockType()) return;
		connection_end_id_t connectionEndId = std::get<1>(data->get());
		auto portToInternalPointMappingIter = portToInternalPointMapping.find(connectionEndId);
		if (portToInternalPointMappingIter == portToInternalPointMapping.end()) return;
		portToInternalPointMappingIter->second.connectionPoint = std::nullopt;
	});
	receiver.linkFunction("circuitBlockDataConnectionPositionSet", [&](const DataUpdateEventManager::EventData* event) {
		const auto* data = event->cast<std::tuple<BlockType, connection_end_id_t, Position>>();
		if (!data) return;
		BlockType blockType = std::get<0>(data->get());
		if (blockType != circuit.getBlockType()) return;
		connection_end_id_t connectionEndId = std::get<1>(data->get());
		const BlockData* blockData = circuitManager.getBlockDataManager().getBlockData(blockType);
		BlockData::ConnectionData::PortType portType = blockData->getConnectionPortType(connectionEndId);
		if (portType == BlockData::ConnectionData::PortType::NONE) return;
		unsigned int bitWidth = blockData->getConnectionBitWidth(connectionEndId);
		assert(bitWidth != 0);
		assert(portType != BlockData::ConnectionData::PortType::BIDIRECTIONAL); // can not happen and if it does some things are undefined
		Position internalPortPosition = std::get<2>(data->get());
		const Block* block = circuit.getBlockContainer().getBlock(internalPortPosition);
		if (!block) {
			portToInternalPointMapping.insert_or_assign(connectionEndId, InternalPointData(portType, bitWidth));
			return;
		}
		auto positionRemappingIter = positionRemapping.find(block->getPosition());
		if (positionRemappingIter == positionRemapping.end()) {
			logError("Could not find positionRemapping while doing \"circuitBlockDataConnectionPositionSet\", not sure why this would happen.", "EvaluatorInternal");
			return;
		}
		const BlockData* internalBlockData = circuitManager.getBlockDataManager().getBlockData(block->type());
		assert(internalBlockData);
		std::optional<connection_end_id_t> internalConnectionEndId;
		if (portType == BlockData::ConnectionData::PortType::INPUT) {
			internalConnectionEndId = internalBlockData->getInputOrBidirectionalConnectionId(
				internalPortPosition - block->getPosition(), block->getOrientation()
			);
		} else {
			internalConnectionEndId = internalBlockData->getOutputOrBidirectionalConnectionId(
				internalPortPosition - block->getPosition(), block->getOrientation()
			);
		}
		if (!internalConnectionEndId.has_value() || bitWidth != internalBlockData->getConnectionBitWidth(internalConnectionEndId.value())) {
			portToInternalPointMapping.insert_or_assign(connectionEndId, InternalPointData(portType, bitWidth));
			return;
			return;
		}

		portToInternalPointMapping.insert_or_assign(
			connectionEndId,
			InternalPointData(EvalConnectionPoint(positionRemappingIter->second.first, internalConnectionEndId.value()), portType, bitWidth)
		);
	});
	receiver.linkFunction("blockDataPortBitConfigurationSet", [&](const DataUpdateEventManager::EventData* event) {
		const auto* data = event->cast<std::tuple<BlockType, connection_end_id_t, unsigned int>>();
		if (!data) return;
		BlockType blockType = std::get<0>(data->get());
		if (blockType != circuit.getBlockType()) return;
		connection_end_id_t connectionEndId = std::get<1>(data->get());
		unsigned int bitWidth = std::get<2>(data->get());
		auto portToInternalPointMappingIter = portToInternalPointMapping.find(connectionEndId);
		if (portToInternalPointMappingIter != portToInternalPointMapping.end()) {
			if (portToInternalPointMappingIter->second.bitWith != bitWidth) {
				portToInternalPointMappingIter->second.bitWith = bitWidth;
				portToInternalPointMappingIter->second.connectionPoint = std::nullopt;
			}
			return;
		}

		const BlockData* blockData = circuitManager.getBlockDataManager().getBlockData(blockType);
		BlockData::ConnectionData::PortType portType = blockData->getConnectionPortType(connectionEndId);
		if (portType == BlockData::ConnectionData::PortType::NONE) return;
		assert(portType != BlockData::ConnectionData::PortType::BIDIRECTIONAL); // can not happen and if it does some things are undefined
		// const CircuitBlockData* circuitBlockData = circuitManager.getCircuitBlockDataManager().getCircuitBlockData(circuit.getCircuitId());
		// assert(circuitBlockData);
		// const Position* internalPortPosition = circuitBlockData->getConnectionIdToPosition(connectionEndId);
		// if (internalPortPosition == nullptr) return;
		// const Block* block = circuit.getBlockContainer().getBlock(*internalPortPosition);
		// if (!block) {
			portToInternalPointMapping.insert_or_assign(connectionEndId, InternalPointData(portType, bitWidth));
			return;
		// }
		// auto positionRemappingIter = positionRemapping.find(block->getPosition());
		// if (positionRemappingIter == positionRemapping.end()) {
		// 	logError("Could not find positionRemapping while doing \"circuitBlockDataConnectionPositionSet\", not sure why this would happen.", "EvaluatorInternal");
		// 	return;
		// }
		// const BlockData* internalBlockData = circuitManager.getBlockDataManager().getBlockData(block->type());
		// assert(internalBlockData);
		// std::optional<connection_end_id_t> internalConnectionEndId;
		// if (portType == BlockData::ConnectionData::PortType::INPUT) {
		// 	internalConnectionEndId = internalBlockData->getInputOrBidirectionalConnectionId(
		// 		*internalPortPosition - block->getPosition(), block->getOrientation()
		// 	);
		// } else {
		// 	internalConnectionEndId = internalBlockData->getOutputOrBidirectionalConnectionId(
		// 		*internalPortPosition - block->getPosition(), block->getOrientation()
		// 	);
		// }
		// if (!internalConnectionEndId.has_value() || bitWidth != internalBlockData->getConnectionBitWidth(internalConnectionEndId.value())) {
		// 	portToInternalPointMapping.insert_or_assign(connectionEndId, InternalPointData(portType, bitWidth));
		// 	return;
		// }

		// portToInternalPointMapping.insert_or_assign(
		// 	connectionEndId,
		// 	InternalPointData(EvalConnectionPoint(positionRemappingIter->second.first, internalConnectionEndId.value()), portType, bitWidth)
		// );
	});
	const CircuitBlockData* circuitBlockData = circuitManager.getCircuitBlockDataManager().getCircuitBlockData(circuit.getCircuitId());
	if (circuitBlockData == nullptr) return;
	const BlockData* blockData = circuitManager.getBlockDataManager().getBlockData(circuitBlockData->getBlockType());
	for (const std::pair<connection_end_id_t, BlockData::ConnectionData> pair : blockData->getConnectionsSafe()) {
		connection_end_id_t connectionEndId = pair.first;
		unsigned int bitWidth = pair.second.getBitWidth();
		BlockData::ConnectionData::PortType portType = pair.second.portType;
		if (portType == BlockData::ConnectionData::PortType::NONE) return;
		assert(portType != BlockData::ConnectionData::PortType::BIDIRECTIONAL); // can not happen and if it does some things are undefined
		const Position* internalPortPosition = circuitBlockData->getConnectionIdToPosition(connectionEndId);
		if (internalPortPosition == nullptr) continue;
		const Block* block = circuit.getBlockContainer().getBlock(*internalPortPosition);
		if (!block) {
			portToInternalPointMapping.try_emplace(connectionEndId, portType, bitWidth);
			continue;
		}
		auto positionRemappingIter = positionRemapping.find(block->getPosition());
		if (positionRemappingIter == positionRemapping.end()) {
			logError("Could not find positionRemapping while doing \"circuitBlockDataConnectionPositionSet\", not sure why this would happen.", "EvaluatorInternal");
			continue;
		}
		const BlockData* internalBlockData = circuitManager.getBlockDataManager().getBlockData(block->type());
		assert(internalBlockData);
		std::optional<connection_end_id_t> internalConnectionEndId;
		if (portType == BlockData::ConnectionData::PortType::INPUT) {
			internalConnectionEndId = internalBlockData->getInputOrBidirectionalConnectionId(
				*internalPortPosition - block->getPosition(), block->getOrientation()
			);
		} else {
			internalConnectionEndId = internalBlockData->getOutputOrBidirectionalConnectionId(
				*internalPortPosition - block->getPosition(), block->getOrientation()
			);
		}
		if (!internalConnectionEndId.has_value() || bitWidth != internalBlockData->getConnectionBitWidth(internalConnectionEndId.value())) {
			portToInternalPointMapping.try_emplace(connectionEndId, portType, bitWidth);
			continue;
		}

		portToInternalPointMapping.try_emplace(
			connectionEndId,
			EvalConnectionPoint(positionRemappingIter->second.first, internalConnectionEndId.value()),
			portType,
			bitWidth
		);
	}
	logInfo(portToInternalPointMapping.size());
}

void EvaluatorInternal::startEdit() {
	layerRunner.getInputLayer().resetEdits();
}

void EvaluatorInternal::endEdit() {
	layerRunner.runAll();
}

void EvaluatorInternal::addBlock(Position position, Orientation orientation, BlockType blockType) {
	eval_gate_id simulatorId = evalGateIdProvider.getNewId();
	positionRemapping.try_emplace(position, simulatorId, orientation);
	positionReverseRemapping.try_emplace(simulatorId, position, orientation);
	if (blockType == BlockType::LIGHT) {
		blockType = BlockType::JUNCTION;
	}
	layerRunner.getInputLayer().addGate(simulatorId, getSimulatorGateType(blockType));
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
	eval_gate_id simulatorId = remappingIter->second.first;
	positionRemapping.erase(remappingIter);
	positionRemapping.try_emplace(newPosition, simulatorId, newOrientation);
	positionReverseRemapping.at(simulatorId) = {newPosition, newOrientation};
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
std::vector<EvalConnectionPoint> EvaluatorInternal::mapFromBottomConnectionPointToTopConnectionPoint(EvalConnectionPoint bottomConnectionPoint) const {
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
std::optional<std::pair<Address, Address>> EvaluatorInternal::mapFromTopConnectionPointToPointAndBlockAddress(EvalConnectionPoint topConnectionPoint) const {
	if (topConnectionPoint.gateId == 0) return std::nullopt;
	auto iter = positionReverseRemapping.find(topConnectionPoint.gateId);
	if (iter == positionReverseRemapping.end()) return std::nullopt;
	return std::make_pair(
		iter->second.first + circuitManager.getBlockDataManager().getConnectionVector(
			getBlockType(layerRunner.getInputLayer().getGate(topConnectionPoint.gateId)->type),
			iter->second.second,
			topConnectionPoint.connectionEndId
		).value(),
		iter->second.first
	);
}
std::vector<Address> EvaluatorInternal::mapFromBottomConnectionPointToAddress(EvalConnectionPoint bottomConnectionPoint) const {
	std::vector<EvalConnectionPoint> topConnectionPoints = mapFromBottomConnectionPointToTopConnectionPoint(bottomConnectionPoint);
	std::vector<Address> addresses;
	for (const EvalConnectionPoint& topConnectionPoint : topConnectionPoints) {
		std::optional<Address> address = mapFromTopConnectionPointToAddress(topConnectionPoint);
		assert(address.has_value());
		addresses.push_back(address.value());
	}
	return addresses;
}
std::vector<std::pair<Address, Address>> EvaluatorInternal::mapFromBottomConnectionPointToAddressAndBlockAddress(EvalConnectionPoint bottomConnectionPoint) const {
	std::vector<EvalConnectionPoint> topConnectionPoints = mapFromBottomConnectionPointToTopConnectionPoint(bottomConnectionPoint);
	std::vector<std::pair<Address, Address>> addressesPairs;
	for (const EvalConnectionPoint& topConnectionPoint : topConnectionPoints) {
		std::optional<std::pair<Address, Address>> pair = mapFromTopConnectionPointToPointAndBlockAddress(topConnectionPoint);
		assert(pair.has_value());
		addressesPairs.push_back(pair.value());
	}
	return addressesPairs;
}
EvalConnectionPoint EvaluatorInternal::mapFromAddressToTopConnectionPoint(const Address& address) const {
	if (address.size() != 1) return EvalConnectionPoint::null();
	assert(mainThreadId == std::this_thread::get_id());
	const Block* block = circuit.getBlockContainer().getBlock(address.getPosition(0));
	if (block == nullptr) return EvalConnectionPoint::null();
	auto iter = positionRemapping.find(block->getPosition());
	if (iter == positionRemapping.end()) return EvalConnectionPoint::null();
	// hardcode some of the v port connections for primitive blocks
	if (block->type() == BlockType::LIGHT || block->type() == BlockType::JUNCTION_H || block->type() == BlockType::JUNCTION_L || block->type() == BlockType::JUNCTION_X) {
		return EvalConnectionPoint(iter->second.first, 0);
	}
	if (block->type() == BlockType::TRISTATE_BUFFER) return EvalConnectionPoint(iter->second.first, 2);
	// other blocks are 1x1 and have a output so you just need to get what connection end id is the output
	std::optional<connection_end_id_t> connectionEndId = block->getOutputOrBidirectionalConnectionId(address.getPosition(0));
	if (!connectionEndId) return  EvalConnectionPoint::null();
	return EvalConnectionPoint(iter->second.first, connectionEndId.value());
}
EvalConnectionPoint EvaluatorInternal::mapFromAddressToBottomConnectionPoint(const Address& address) const {
	EvalConnectionPoint topConnectionPoint = mapFromAddressToTopConnectionPoint(address);
	if (topConnectionPoint.isNull()) return EvalConnectionPoint::null();
	return mapFromTopConnectionPointToBottomConnectionPoint(topConnectionPoint);
}
