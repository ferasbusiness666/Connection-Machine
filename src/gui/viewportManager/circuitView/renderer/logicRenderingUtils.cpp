#include "logicRenderingUtils.h"

constexpr float edgeDistance = 0.48f;
constexpr float sideShift = 0.25f;

FVector getOutputOffset(BlockType blockType, connection_end_id_t connectionEndId) {
	return getOutputOffset(blockType, connectionEndId, Orientation());
}

FVector getInputOffset(BlockType blockType, connection_end_id_t connectionEndId) {
	return getInputOffset(blockType, connectionEndId, Orientation());
}

FVector getOutputOffset(BlockType blockType, connection_end_id_t connectionEndId, Orientation orientation) {
	// TODO: make this part of render block data
	FVector offset = { 0.5, 0.5 };
	if (blockType == BlockType::JUNCTION) return offset;
	if (blockType == BlockType::BUS_INTERFACE_1 || blockType == BlockType::BUS_INTERFACE_2 || blockType == BlockType::BUS_INTERFACE_3) {
		if (connectionEndId == 0) {
			return offset + orientation * FVector(edgeDistance, 0.0f);
		} else {
			return offset + orientation * FVector(-edgeDistance, 0.0f);
		}
	}

	return offset + orientation * FVector(edgeDistance, sideShift);
}

FVector getInputOffset(BlockType blockType, connection_end_id_t connectionEndId, Orientation orientation) {
	// TODO: make this part of render block data
	FVector offset = { 0.5, 0.5 };
	if (blockType == BlockType::JUNCTION) return offset;
	if (blockType == BlockType::BUS_INTERFACE_1 || blockType == BlockType::BUS_INTERFACE_2 || blockType == BlockType::BUS_INTERFACE_3) {
		if (connectionEndId == 1) {
			return offset + orientation * FVector(edgeDistance, 0.0f);
		} else {
			return offset + orientation * FVector(-edgeDistance, 0.0f);
		}
	}

	return offset + orientation * FVector(-edgeDistance, -sideShift);
}
