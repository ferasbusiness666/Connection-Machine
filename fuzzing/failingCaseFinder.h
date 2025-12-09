#ifndef failingCaseFinder_h
#define failingCaseFinder_h

#include "fuzzTestcase.h"
class FailingCaseFinder {
public:
	FailingCaseFinder() = default;

	std::unique_ptr<FuzzTestcase> findFailingCases(unsigned int maxAttampts, const std::vector<FuzzBlockType>& blockTypesUsed);
private:
	std::unique_ptr<FuzzTestcase> tryMakeFailingCase(const std::vector<FuzzBlockType>& blockTypesUsed);
};

#endif /* failingCaseFinder_h */