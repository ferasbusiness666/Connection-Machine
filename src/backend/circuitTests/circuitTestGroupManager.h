#ifndef circuitTestGroupManager_h
#define circuitTestGroupManager_h

#include "backend/circuitTests/circuitTestGroup.h"

class CircuitTestGroupManager {
public:
    CircuitTestGroupManager() = default;
	CircuitTestGroupManager(const CircuitTestGroupManager&) = delete;
    CircuitTestGroupManager& operator=(const CircuitTestGroupManager&) = delete;

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

    typedef std::unordered_map<std::string, CircuitTestGroup>::iterator iterator;
	typedef std::unordered_map<std::string, CircuitTestGroup>::const_iterator const_iterator;

    inline iterator begin() { return testGroups.begin(); }
	inline iterator end() { return testGroups.end(); }
	inline const_iterator begin() const { return testGroups.begin(); }
	inline const_iterator end() const { return testGroups.end(); }
	inline int getTestCount() const { return testGroups.size(); }


private:
    std::unordered_map<std::string, CircuitTestGroup> testGroups;
};

#endif /* circuitTestGroupManager_h */