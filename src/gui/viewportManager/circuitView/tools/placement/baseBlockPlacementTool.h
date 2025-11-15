#ifndef baseBlockPlacementTool_h
#define baseBlockPlacementTool_h

#include "../circuitToolHelper.h"
#include "backend/position/position.h"

class BaseBlockPlacementTool : public CircuitToolHelper {
public:
	using CircuitToolHelper::CircuitToolHelper;
	// This will also tell the tool to reset.
	void selectBlock(BlockType selectedBlock) { this->selectedBlock = selectedBlock; updateElements(); }
	void setOrientation(Orientation orientation) { this->orientation = orientation; updateElements(); }
	Orientation getOrientation() const { return orientation; }

	void activate() override;

	bool rotateBlockCW(const Event* event);
	bool rotateBlockCCW(const Event* event);
	bool flipBlock(const Event* event);

protected:
	BlockType selectedBlock = BlockType::NONE;
	Orientation orientation = Orientation();
};

typedef std::shared_ptr<BaseBlockPlacementTool> SharedBaseBlockPlacementTool;

#endif /* baseBlockPlacementTool_h */
