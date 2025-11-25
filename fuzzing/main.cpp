#include "failingCaseFinder.h"
#include "testcaseMinifier.h"
#include "fuzzTestcase.h"

#include "computerAPI/directoryManager.h"

int main(int argc, char** argv) {
	DirectoryManager::findDirectories();
	std::string failingTestcasePath = (DirectoryManager::getConfigDirectory() / "fuzzing" / "failing_testcase.json").string();
	FailingCaseFinder finder;
	std::vector<FuzzBlockType> blockTypesUsed = {
		FuzzPrimitiveType { BlockType::AND },
		FuzzPrimitiveType { BlockType::OR },
		FuzzPrimitiveType { BlockType::XOR },
		FuzzPrimitiveType { BlockType::NAND },
		FuzzPrimitiveType { BlockType::NOR },
		FuzzPrimitiveType { BlockType::XNOR },
		FuzzPrimitiveType { BlockType::JUNCTION },
		FuzzPrimitiveType { BlockType::SWITCH },
		FuzzPrimitiveType { BlockType::CONSTANT_OFF },
		FuzzPrimitiveType { BlockType::CONSTANT_ON },
		FuzzPrimitiveType { BlockType::CONSTANT_Z },
		FuzzPrimitiveType { BlockType::CONSTANT_X },
	};
	std::unique_ptr<FuzzTestcase> testcase = finder.findFailingCases(1000, blockTypesUsed);
	if (testcase) {
		logInfo("Found failing testcase with {} edit actions and {} test actions", "", testcase->getEditActions().size(), testcase->getTestActions().size());
		TestcaseMinifier minifier;
		FuzzTestcase minifiedTestcase = minifier.minifyTestcase(*testcase);
		logInfo("Minified testcase has {} edit actions and {} test actions", "", minifiedTestcase.getEditActions().size(), minifiedTestcase.getTestActions().size());
		std::string serialized = minifiedTestcase.serialize();
		std::ofstream outFile(failingTestcasePath);
		outFile << serialized;
		outFile.close();
		logInfo("Serialized failing testcase written to {}", "", failingTestcasePath);
	} else {
		logInfo("No failing testcase found");
	}
	return 0;
}
