#include "circuitTestGroupManager.h"
#include "backend/circuitTests/circuitTestGroup.h"

bool CircuitTestGroupManager::addCircuitTestGroup(CircuitTestGroup&& testGroup) {
    std::string name = testGroup.getName();
    testGroups.emplace(name, std::move(testGroup));
}