#include "failingCaseFinder.h"
#include "testcaseMinifier.h"
#include "fuzzTestcase.h"

int main(int argc, char** argv) {
	FailingCaseFinder finder;
	std::unique_ptr<FuzzTestcase> testcase = finder.findFailingCases(1000);
	if (testcase) {
		logInfo("Found failing testcase with {} edit actions and {} test actions", "", testcase->getEditActions().size(), testcase->getTestActions().size());
		TestcaseMinifier minifier;
		FuzzTestcase minifiedTestcase = minifier.minifyTestcase(*testcase);
		logInfo("Minified testcase has {} edit actions and {} test actions", "", minifiedTestcase.getEditActions().size(), minifiedTestcase.getTestActions().size());
	} else {
		logInfo("No failing testcase found");
	}
	return 0;
}
