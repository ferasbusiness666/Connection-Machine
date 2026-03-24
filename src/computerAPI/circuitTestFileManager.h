#ifndef circuitTestFileManager_h
#define circuitTestFileManager_h

#include "backend/circuitTests/circuitTestGroup.h"

namespace CircuitTestFileManager {
    std::optional<CircuitTestGroup> getCircuitTestFromFilePath(const std::string& path);
}

#endif /* circuitTestFileManager_h */