#ifndef testcaseMinifier_h
#define testcaseMinifier_h

class FuzzTestcase;

class TestcaseMinifier {
public:
	TestcaseMinifier() = default;
	FuzzTestcase minifyTestcase(const FuzzTestcase& originalTestcase);

private:
	std::unique_ptr<FuzzTestcase> tryRemoveEditActions(const FuzzTestcase& testcase, std::set<size_t> editActions, std::set<size_t> testActions);
};

#endif /* testcaseMinifier_h */