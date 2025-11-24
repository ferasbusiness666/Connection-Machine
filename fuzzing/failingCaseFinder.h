#ifndef failingCaseFinder_h
#define failingCaseFinder_h

class FuzzTestcase;

class FailingCaseFinder {
public:
	FailingCaseFinder() = default;

	std::unique_ptr<FuzzTestcase> findFailingCases(unsigned int maxAttampts);
private:
	std::unique_ptr<FuzzTestcase> tryMakeFailingCase();
};

#endif /* failingCaseFinder_h */