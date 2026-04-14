#ifndef circuitRenderManager_h
#define circuitRenderManager_h

#include "backend/container/difference.h"
#include "backend/circuit/circuitDefs.h"
#include "gpu/mainRendererDefs.h"

class Environment;
class Backend;

class CircuitRenderManager {
public:
	CircuitRenderManager(Environment& environment, circuit_id_t circuitId, ViewportId viewportId);
	~CircuitRenderManager();
	void addDifference(DifferenceSharedPtr diff);

private:
	struct RenderedBlock {
		RenderedBlock(BlockType type, Orientation orientation) : type(type), orientation(orientation) {}
		std::unordered_map<std::pair<Position, Position>, Position> connectionsToOtherBlock;
		BlockType type;
		Orientation orientation;
	};

	void createPortConnection(Position blockPosition, Position portPosition, const std::vector<std::pair<Position, Position>>& otherBlockPositions);

	circuit_id_t circuitId;
	ViewportId viewportId;
	Environment& environment;
	Backend& backend;

	std::unordered_map<Position, RenderedBlock> renderedBlocks;
};

#endif /* circuitRenderManager_h */
