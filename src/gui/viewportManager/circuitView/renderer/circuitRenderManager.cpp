#include "circuitRenderManager.h"

#ifdef TRACY_PROFILER
#include <tracy/Tracy.hpp>
#endif

#include "gpu/mainRenderer.h"
#include "backend/circuit/circuit.h"
#include "logicRenderingUtils.h"

CircuitRenderManager::CircuitRenderManager(Circuit* circuit, ViewportId viewportId) : circuit(circuit), viewportId(viewportId) {
	circuit->connectListener(this, [this](DifferenceSharedPtr diff, circuit_id_t circuitId) {if (circuitId == this->circuit->getCircuitId()) addDifference(diff); });
	addDifference(circuit->getBlockContainer()->getCreationDifferenceShared());
}

CircuitRenderManager::~CircuitRenderManager() {
	MainRenderer::get().resetCircuit(viewportId);
	circuit->disconnectListener(this);
}

void CircuitRenderManager::addDifference(DifferenceSharedPtr diff) {
#ifdef TRACY_PROFILER
	ZoneScoped;
#endif
	if (diff->clearsAll()) {
		renderedBlocks.clear();
		MainRenderer::get().resetCircuit(viewportId);
		return;
	}

	MainRenderer::get().startMakingEdits(viewportId);

	const BlockDataManager* blockDataManager = circuit->getBlockContainer()->getBlockDataManager();
	for (const auto& modification : diff->getModifications()) {
		const auto& [modificationType, modificationData] = modification;
		switch (modificationType) {
		case Difference::ModificationType::PLACE_BLOCK:
		{
			const auto& [position, orientation, blockType] = std::get<Difference::block_modification_t>(modificationData);
			Position statePosition = Position(1000000, 1000000);
			if (blockType < BlockType::CUSTOM) {
				if (blockType == BlockType::TRISTATE_BUFFER) statePosition = position + orientation.transformVectorWithArea(Vector(0, 1), Size(1, 2));
				else statePosition = position;
			}
			MainRenderer::get().addBlock(viewportId, blockType, position, orientation, statePosition);
			renderedBlocks.emplace(position, RenderedBlock(blockType, orientation));
			break;
		}
		case Difference::ModificationType::REMOVED_BLOCK:
		{
			const auto& [position, orientation, blockType] = std::get<Difference::block_modification_t>(modificationData);

			MainRenderer::get().removeBlock(viewportId, position);
			auto iter = renderedBlocks.find(position);
			if (iter == renderedBlocks.end()) {
				logError("Could not find block at {} to remove.", "CircuitRenderManager", position);
				continue;
			}
			if (iter->second.connectionsToOtherBlock.size() != 0) {
				logError("Trying to remove block at {} that still has {} connections.", "CircuitRenderManager", position, iter->second.connectionsToOtherBlock.size());
				continue;
			}
			renderedBlocks.erase(iter);
			break;
		}
		case Difference::ModificationType::CREATED_CONNECTION:
		{
			const auto& [outputBlockPosition, outputPosition, inputBlockPosition, inputPosition] = std::get<Difference::connection_modification_t>(modificationData);

			// uses position of output and input CELLS fed into offset function
			std::pair<Position, Position> newConnection = { outputPosition, inputPosition };

			auto outputIter = renderedBlocks.find(outputBlockPosition);
			if (outputIter == renderedBlocks.end()) {
				logError("Could not find block at {} to add output connection to.", "CircuitRenderManager", outputBlockPosition);
			}
			outputIter->second.connectionsToOtherBlock.emplace(newConnection, inputBlockPosition);

			connection_end_id_t outputEndId = circuit->getBlockContainer()->getOutputConnectionEnd(outputPosition).value().getConnectionId();
			connection_end_id_t inputEndId = circuit->getBlockContainer()->getInputConnectionEnd(inputPosition).value().getConnectionId();

			// only need both if it is a different block
			if (outputBlockPosition != inputBlockPosition) {
				auto inputIter = renderedBlocks.find(inputBlockPosition);
				if (inputIter == renderedBlocks.end()) {
					logError("Could not find block at {} to add input connection to.", "CircuitRenderManager", inputBlockPosition);
				}
				inputIter->second.connectionsToOtherBlock.emplace(newConnection, outputBlockPosition);
				MainRenderer::get().addWire(viewportId, newConnection, {
					getOutputOffset(outputIter->second.type, outputEndId, outputIter->second.orientation),
					getInputOffset(inputIter->second.type, inputEndId, inputIter->second.orientation)
				});
			} else {
				MainRenderer::get().addWire(viewportId, newConnection, {
					getOutputOffset(outputIter->second.type, outputEndId, outputIter->second.orientation),
					getInputOffset(outputIter->second.type, inputEndId, outputIter->second.orientation)
				});
			}
			break;
		}
		case Difference::ModificationType::REMOVED_CONNECTION:
		{
			const auto& [outputBlockPosition, outputPosition, inputBlockPosition, inputPosition] = std::get<Difference::connection_modification_t>(modificationData);
			MainRenderer::get().removeWire(viewportId, { outputPosition, inputPosition });

			auto outputIter = renderedBlocks.find(outputBlockPosition);
			if (outputIter == renderedBlocks.end()) {
				logError("Could not find block at {} to remove output connection from.", "CircuitRenderManager", outputBlockPosition);
			}
			outputIter->second.connectionsToOtherBlock.erase({ outputPosition, inputPosition });

			// only need both if it is a different block
			if (outputBlockPosition != inputBlockPosition) {
				auto inputIter = renderedBlocks.find(inputBlockPosition);
				if (inputIter == renderedBlocks.end()) {
					logError("Could not find block at {} to remove input connection from.", "CircuitRenderManager", inputBlockPosition);
				}
				inputIter->second.connectionsToOtherBlock.erase({ outputPosition, inputPosition });
			}
			break;
		}
		case Difference::ModificationType::MOVE_BLOCK:
		{
			const auto& [curPosition, curOrientation, newPosition, newOrientation, moveType] = std::get<Difference::move_modification_t>(modificationData);
			logInfo("moving {} {} {} {} {}", "CircuitRenderManager", curPosition, curOrientation, newPosition, newOrientation, (unsigned int)moveType);
			if (curPosition == newPosition && curOrientation == newOrientation) continue;

			auto iter = renderedBlocks.find(curPosition);
			if (iter == renderedBlocks.end()) {
				logError("Could not find block at {} to move.", "CircuitRenderManager", curPosition);
				continue;
			}

			if (curPosition != newPosition) {
				auto newIter = renderedBlocks.emplace(newPosition, std::move(iter->second)).first;
				renderedBlocks.erase(iter);
				iter = newIter;
			}

			iter->second.orientation = newOrientation;

			// MOVE BLOCK
			MainRenderer::get().moveBlock(viewportId, curPosition, newPosition, newOrientation);

			Size blockSize = blockDataManager->getBlockSize(iter->second.type, curOrientation);
			Orientation transformAmount = newOrientation.relativeTo(curOrientation);

			// MOVE CONNECTIONS
			std::unordered_map<std::pair<Position, Position>, Position> oldConnectionsToOtherBlock = std::move(iter->second.connectionsToOtherBlock);
			iter->second.connectionsToOtherBlock.clear();

			std::vector<std::pair<std::pair<Position, Position>, std::pair<Position, Position>>> wiresToAdd;
			std::vector<std::pair<std::pair<Position, Position>, std::pair<FVector, FVector>>> wiresToRender;

			for (auto& [posPair, otherBlockPos] : oldConnectionsToOtherBlock) {
				if ((moveType == MoveType::SINGLE || moveType == MoveType::MULTI_BEGIN) && otherBlockPos.x != 10000000) {
					MainRenderer::get().removeWire(viewportId, posPair);
				}
				if (otherBlockPos == curPosition) {
					Vector outputVec = transformAmount.transformVectorWithArea(posPair.first - curPosition, blockSize);
					Position outputPos = newPosition + outputVec;
					Vector inputVec = transformAmount.transformVectorWithArea(posPair.second - curPosition, blockSize);
					Position inputPos = newPosition + inputVec;
					if (moveType == MoveType::SINGLE || moveType == MoveType::MULTI_FINAL) {
						connection_end_id_t outputEndId = circuit->getBlockContainer()->getBlockDataManager()->getOutputConnectionId(
							iter->second.type, newOrientation, outputVec
						).value();
						connection_end_id_t inputEndId = circuit->getBlockContainer()->getBlockDataManager()->getInputConnectionId(
							iter->second.type, newOrientation, inputVec
						).value();
						wiresToRender.emplace_back(std::make_pair(outputPos, inputPos), std::make_pair(
							getOutputOffset(iter->second.type, outputEndId, newOrientation),
							getInputOffset(iter->second.type, inputEndId, newOrientation)
						));
					}
					wiresToAdd.emplace_back(std::make_pair(outputPos, inputPos), std::make_pair(newPosition, newPosition));
				} else {
					auto otherIter = renderedBlocks.find(otherBlockPos);
					if (otherIter == renderedBlocks.end()) {
						logError("Could not find block at {} to move connection.", "CircuitRenderManager", otherBlockPos);
						continue;
					}
					otherIter->second.connectionsToOtherBlock.erase(posPair);
					bool isInput = posPair.second.withinArea(curPosition, curPosition + blockSize.getLargestVectorInArea());
					if (isInput) {
						Vector inputVec = transformAmount.transformVectorWithArea(posPair.second - curPosition, blockSize);
						Position inputPos = newPosition + inputVec;
						if ((moveType == MoveType::SINGLE || moveType == MoveType::MULTI_FINAL) && otherBlockPos.x != 10000000) {
							connection_end_id_t outputEndId = circuit->getBlockContainer()->getBlockDataManager()->getOutputConnectionId(
								otherIter->second.type, otherIter->second.orientation, posPair.first - otherBlockPos
							).value();
							connection_end_id_t inputEndId = circuit->getBlockContainer()->getBlockDataManager()->getInputConnectionId(
								iter->second.type, newOrientation, inputVec
							).value();
							wiresToRender.emplace_back(std::make_pair(posPair.first, inputPos), std::make_pair(
								getOutputOffset(otherIter->second.type, outputEndId, otherIter->second.orientation),
								getInputOffset(iter->second.type, inputEndId, newOrientation)
							));
						}
						wiresToAdd.emplace_back(std::make_pair(posPair.first, inputPos), std::make_pair(otherBlockPos, newPosition));
					} else {
						Vector outputVec = transformAmount.transformVectorWithArea(posPair.first - curPosition, blockSize);
						Position outputPos = newPosition + outputVec;
						if ((moveType == MoveType::SINGLE || moveType == MoveType::MULTI_FINAL) && otherBlockPos.x != 10000000) {
							connection_end_id_t outputEndId = circuit->getBlockContainer()->getBlockDataManager()->getOutputConnectionId(
								iter->second.type, newOrientation, outputVec
							).value();
							connection_end_id_t inputEndId = circuit->getBlockContainer()->getBlockDataManager()->getInputConnectionId(
								otherIter->second.type, otherIter->second.orientation, posPair.second - otherBlockPos
							).value();
							wiresToRender.emplace_back(std::make_pair(outputPos, posPair.second), std::make_pair(
								getOutputOffset(iter->second.type, outputEndId, newOrientation),
								getInputOffset(otherIter->second.type, inputEndId, otherIter->second.orientation)
							));
						}
						wiresToAdd.emplace_back(std::make_pair(outputPos, posPair.second), std::make_pair(newPosition, otherBlockPos));
					}
				}
			}
			for (const auto& pairs : wiresToAdd) {
				if (pairs.second.first == pairs.second.second) {
					renderedBlocks.find(pairs.second.first)->second.connectionsToOtherBlock.emplace(pairs.first, pairs.second.first);
				} else {
					renderedBlocks.find(pairs.second.first)->second.connectionsToOtherBlock.emplace(pairs.first, pairs.second.second);
					renderedBlocks.find(pairs.second.second)->second.connectionsToOtherBlock.emplace(pairs.first, pairs.second.first);
				}
			}
			for (const auto& pairs : wiresToRender) {
				MainRenderer::get().addWire(viewportId, pairs.first, pairs.second);
			}
		}
		}
	}

	MainRenderer::get().stopMakingEdits(viewportId);
}
