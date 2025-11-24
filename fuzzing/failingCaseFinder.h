#ifndef failingCaseFinder_h
#define failingCaseFinder_h

#include "fuzzTestcase.h"

class FailingCaseFinder {
public:
	FailingCaseFinder() = default;

	std::unique_ptr<FuzzTestcase> findFailingCases(unsigned int maxAttampts);
private:
	std::unique_ptr<FuzzTestcase> tryMakeFailingCase();
};

#endif /* failingCaseFinder_h */