#ifndef circuitTestGroupManager_h
#define circuitTestGroupManager_h

#include "backend/circuitTests/circuitTestGroup.h"

class CircuitTestGroupManager {
public:
    bool addCircuitTestGroup(CircuitTestGroup&& testGroup);
    CircuitTestGroup* getCircuitTestGroup(std::string name) {
        auto iter = testGroups.find(name);
		if (iter == testGroups.end()) return nullptr;
		return &iter->second;
    }
    const CircuitTestGroup* getCircuitTestGroup(std::string name) const {
        auto iter = testGroups.find(name);
		if (iter == testGroups.end()) return nullptr;
		return &iter->second;
    } 

private:
    std::unordered_map<std::string, CircuitTestGroup> testGroups;
};

#endif /* circuitTestGroupManager_h */