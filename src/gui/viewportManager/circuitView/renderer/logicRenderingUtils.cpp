#include "logicRenderingUtils.h"

constexpr float edgeDistance = 0.48f;
constexpr float sideShift = 0.25f;

FVector getOutputOffset(std::pair<BlockType, connection_end_id_t> connection) {
	return getOutputOffset(connection, Orientation());
}

FVector getInputOffset(std::pair<BlockType, connection_end_id_t> connection) {
	return getInputOffset(connection, Orientation());
}

FVector getOutputOffset(std::pair<BlockType, connection_end_id_t> connection, Orientation orientation) {
	FVector offset = { 0.5, 0.5 };
	if (connection.first == BlockType::JUNCTION) return offset;
	if (connection.first == BlockType::BUS_INTERFACE) {
		if (connection.second == 0) {
			return offset + orientation * FVector(edgeDistance, 0.0f);
		} else {
			return offset + orientation * FVector(-edgeDistance, 0.0f);
		}
	}

	return offset + orientation * FVector(edgeDistance, sideShift);
}

FVector getInputOffset(std::pair<BlockType, connection_end_id_t> connection, Orientation orientation) {
	FVector offset = { 0.5, 0.5 };
	if (connection.first == BlockType::JUNCTION) return offset;
	if (connection.first == BlockType::BUS_INTERFACE) {
		if (connection.second == 1) {
			return offset + orientation * FVector(edgeDistance, 0.0f);
		} else {
			return offset + orientation * FVector(-edgeDistance, 0.0f);
		}
	}

	return offset + orientation * FVector(-edgeDistance, -sideShift);
}
