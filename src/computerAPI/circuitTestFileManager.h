#ifndef circuitTestFileManager_h
#define circuitTestFileManager_h

#include "backend/circuitTests/circuitTestGroup.h"

namespace CircuitTestFileManager {
    std::optional<CircuitTestGroup> getCircuitTestGroupFromFilePath(const std::string& path);
    std::optional<CircuitTestGroup> getCircuitTestGroupFromTruthTableFilePath(const std::string& path);

    bool saveToTruthTableFile(const std::string& path, CircuitTestGroup& testGroup);
    bool saveToFile(const std::string& path, CircuitTestGroup& testGroup);
}

#endif /* circuitTestFileManager_h */