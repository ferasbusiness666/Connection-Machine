#ifndef logicRenderingUtils_h
#define logicRenderingUtils_h

#include "backend/position/position.h"
#include "backend/container/block/blockDefs.h"
#include "backend/container/block/connectionEnd.h"

FVector getOutputOffset(BlockType blockType, connection_end_id_t connectionEndId);
FVector getInputOffset(BlockType blockType, connection_end_id_t connectionEndId);
FVector getOutputOffset(BlockType blockType, connection_end_id_t connectionEndId, Orientation orientation);
FVector getInputOffset(BlockType blockType, connection_end_id_t connectionEndId, Orientation orientation);

#endif
