#ifndef circuitTestFileManager_h
#define circuitTestFileManager_h

#include "backend/circuitTests/circuitTestGroup.h"
#include "environment/environment.h"

namespace CircuitTestFileManager {
    std::optional<CircuitTestGroup> getCircuitTestGroupFromFilePath(const std::string& path, Environment& environment);
    std::optional<CircuitTestGroup> getCircuitTestGroupFromTruthTableFilePath(const std::string& path, Environment& environment);

    bool saveToTruthTableFile(const std::string& path, CircuitTestGroup& testGroup);
    bool saveToFile(const std::string& path, CircuitTestGroup& testGroup);
}

#endif /* circuitTestFileManager_h */