#ifndef logicRenderingUtils_h
#define logicRenderingUtils_h

#include "backend/position/position.h"
#include "backend/container/block/blockDefs.h"
#include "backend/container/block/connectionEnd.h"

FVector getOutputOffset(std::pair<BlockType, connection_end_id_t> connection);
FVector getInputOffset(std::pair<BlockType, connection_end_id_t> connection);
FVector getOutputOffset(std::pair<BlockType, connection_end_id_t> connection, Orientation orientation);
FVector getInputOffset(std::pair<BlockType, connection_end_id_t> connection, Orientation orientation);

#endif
