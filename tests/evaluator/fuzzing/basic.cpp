#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"
#include "backend/container/block/blockDefs.h"
#include "backend/blockData/blockDataManager.h"
#include "computerAPI/directoryManager.h"

namespace {

	struct BusDef {
		unsigned int numInputs;
		unsigned int numOutputs;
		unsigned int inputLaneWidth;
		unsigned int outputLaneWidth;
	};

	struct RunningConfig {
		unsigned int numEditOperations;
		unsigned int numTestOperations;
		unsigned int numTicksBetweenTests;
		unsigned int numStatesSetPerTest;
	};

	struct BlockTypesAllowed {
		std::vector<BlockType> blockTypesToUse;
		std::vector<BusDef> busDefinitions;
		std::vector<std::string> customBlockPaths;
		std::string hash() const {
			std::string h;
			for (BlockType bt : blockTypesToUse) {
				h += std::to_string(static_cast<uint16_t>(bt)) + "-";
			}
			for (const BusDef& busDef : busDefinitions) {
				h += "b" + std::to_string(busDef.numInputs) + "_" + std::to_string(busDef.numOutputs) + "_" +
					std::to_string(busDef.inputLaneWidth) + "_" + std::to_string(busDef.outputLaneWidth) + "-";
			}
			for (const std::string& path : customBlockPaths) {
				h += "c" + path + "-";
			}
			return h;
		};
	};

	struct TestcaseConfig {
		uint64_t seed;
		bool realistic;
		RunningConfig runningConfig;
		BlockTypesAllowed blockTypesAllowed;
		int index;
	};

	std::string testcase_config_to_string(const TestcaseConfig& config) {
		std::string str = std::string("s_") + std::to_string(config.seed);
		str += "_r_";
		str += (config.realistic ? "1" : "0");
		RunningConfig runConfig = config.runningConfig;
		str += "_eo_" + std::to_string(runConfig.numEditOperations);
		str += "_to_" + std::to_string(runConfig.numTestOperations);
		str += "_tbt_" + std::to_string(runConfig.numTicksBetweenTests);
		str += "_spt_" + std::to_string(runConfig.numStatesSetPerTest);
		str += "_bt_" + config.blockTypesAllowed.hash();
		return std::to_string(std::hash<std::string>{}(str));
	};

	int u = 0;
	std::vector<TestcaseConfig> makeTestcases() {
		std::vector<TestcaseConfig> testcases;
		for (uint64_t seed = 0; seed < 50; ++seed) {
			for (bool realistic : {false, true}) {
				// RunningConfig runningConfig = {5000, 100, 3, 50};
				RunningConfig runningConfig = {100, 50, 3, 20};
				BlockTypesAllowed blockTypesAllowed = {
					{
						BlockType::AND,
						BlockType::OR,
						BlockType::XOR,
						BlockType::NAND,
						BlockType::NOR,
						BlockType::XNOR,
						BlockType::BUFFER,
						BlockType::NOT,
						BlockType::TRISTATE_BUFFER,
						BlockType::JUNCTION,
						BlockType::JUNCTION_L,
						BlockType::JUNCTION_H,
						BlockType::JUNCTION_X,
						BlockType::LIGHT,
						BlockType::SWITCH,
						BlockType::BUTTON
					}, {}, {}
				};
				testcases.push_back(TestcaseConfig { seed, realistic, runningConfig, blockTypesAllowed, u++ });

				blockTypesAllowed = {
					{}, {}, {}
				};
				for (unsigned int i = 1; i <= 21; ++i) {
					blockTypesAllowed.blockTypesToUse.push_back(BlockType(i));
				}
				blockTypesAllowed.busDefinitions.push_back(BusDef {2, 1, 1, 2});
				blockTypesAllowed.busDefinitions.push_back(BusDef {4, 1, 1, 4});
				blockTypesAllowed.busDefinitions.push_back(BusDef {2, 1, 2, 4});
				blockTypesAllowed.busDefinitions.push_back(BusDef {8, 1, 1, 8});
				blockTypesAllowed.busDefinitions.push_back(BusDef {4, 1, 2, 8});
				blockTypesAllowed.busDefinitions.push_back(BusDef {2, 1, 4, 8});
				blockTypesAllowed.busDefinitions.push_back(BusDef { 2, 4, 4, 2 });

				blockTypesAllowed.customBlockPaths.push_back("passthrough.cir");
				blockTypesAllowed.customBlockPaths.push_back("full_adder.cir");
				blockTypesAllowed.customBlockPaths.push_back("bus_tristate_2.cir");
				blockTypesAllowed.customBlockPaths.push_back("nested_passthrough.cir");
				testcases.push_back(TestcaseConfig { seed, realistic, runningConfig, blockTypesAllowed, u++ });
			}
		}
		// return { testcases.at(33) };//, testcases.at(35), testcases.at(84), testcases.at(86), testcases.at(148) };
		// return {testcases.at(80)};
		return testcases;
	}
}; // namespace 

class BasicFuzzingEvaluatorTest : public ::testing::TestWithParam<TestcaseConfig> {
protected:
	void SetUp() override;
	void TearDown() override;
	std::mt19937_64 gen;
	Environment environment {false};
	SharedCircuit circuit = nullptr;
	SharedEvaluator tEval = nullptr; // testing evaluator
	SharedEvaluator rEval = nullptr; // reference evaluator
	BlockType loadCircuit(const std::filesystem::path& path);
};

BlockType BasicFuzzingEvaluatorTest::loadCircuit(const std::filesystem::path& path) {
	CircuitFileManager& circuitFileManager = environment.getCircuitFileManager();
	circuit_id_t circuitId = circuitFileManager.loadFromFile(path.string()).at(0);
	SharedCircuit circuit = environment.getBackend().getCircuitManager().getCircuit(circuitId);
	return circuit->getBlockType();
}

void BasicFuzzingEvaluatorTest::SetUp() {
	circuit_id_t circuitId = environment.getBackend().getCircuitManager().createNewCircuit(false);
	circuit = environment.getBackend().getCircuit(circuitId);
	evaluator_id_t evalId = environment.getBackend().createEvaluator(circuitId).value();
	tEval = environment.getBackend().getEvaluator(evalId);
	ASSERT_TRUE(tEval->isPause());
}

void BasicFuzzingEvaluatorTest::TearDown() {
	circuit.reset();
	tEval.reset();
	rEval.reset();
}

std::optional<connection_end_id_t> getRandomConnectionEnd(const BlockData* blockData, std::mt19937_64& gen, bool wantInput) {
	if (blockData->isDefaultData()) {
		if (wantInput) {
			return connection_end_id_t(0);
		} else {
			return connection_end_id_t(1);
		}
	}
	int numConnections = blockData->getBidirectionalConnectionCount().get();
	if (wantInput) numConnections += blockData->getInputConnectionCount().get();
	else numConnections += blockData->getOutputConnectionCount().get();
	if (numConnections == 0) return std::nullopt;
	std::uniform_int_distribution<int> dist(0, numConnections - 1);
	int index = dist(gen);
	const std::unordered_map<connection_end_id_t, BlockData::ConnectionData>& connections = blockData->getConnections();
	int i = 0;
	for (auto& pair : connections) {
		if (wantInput) {
			if (pair.second.portType == BlockData::ConnectionData::PortType::INPUT || pair.second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
				if (i == index) return pair.first;
				i++;
			}
		} else {
			if (pair.second.portType == BlockData::ConnectionData::PortType::OUTPUT || pair.second.portType == BlockData::ConnectionData::PortType::BIDIRECTIONAL) {
				if (i == index) return pair.first;
				i++;
			}
		}
	}
	return std::nullopt;
}

TEST_P(BasicFuzzingEvaluatorTest, FuzzInteractions) {
	TestcaseConfig config = GetParam();
	gen.seed(config.seed);
	bool runRealistic = config.realistic;
	tEval->setRealistic(runRealistic);
	BlockDataManager& blockDataManager = environment.getBackend().getBlockDataManager();
	std::uniform_int_distribution<int> distPos(-20, 20);
	std::vector<block_id_t> blockIds;
	RunningConfig runningConfig = config.runningConfig;
	int numEditOperations = runningConfig.numEditOperations;
	int numTestOperations = runningConfig.numTestOperations;
	int numTicksBetweenTests = runningConfig.numTicksBetweenTests;
	int numStatesSetPerTest = runningConfig.numStatesSetPerTest;
	BlockTypesAllowed typeConfig = config.blockTypesAllowed;
	std::vector<BlockType> allowedBlockTypes = typeConfig.blockTypesToUse;
	for (BusDef& busDef : typeConfig.busDefinitions) {
		BlockType busBlockType = blockDataManager.getBusBlock(
			busDef.numInputs,
			busDef.numOutputs,
			busDef.inputLaneWidth,
			busDef.outputLaneWidth
		);
		allowedBlockTypes.push_back(busBlockType);
	}
	for (const std::string& customBlockPath : typeConfig.customBlockPaths) {
		BlockType customBlockType = loadCircuit(DirectoryManager::getResourceDirectory() / "circuits" / "evaluator" / customBlockPath);
		allowedBlockTypes.push_back(customBlockType);
	}
	logInfo("Fuzzing test with seed {}, realistic {}, {} edit operations, {} test operations, {} ticks between tests, {} states per test, {} allowed block types. config idx: {}", "BasicFuzzingEvaluatorTest", config.seed, runRealistic, numEditOperations, numTestOperations, numTicksBetweenTests, numStatesSetPerTest, allowedBlockTypes.size(), config.index);
	for (int i = 0; i < numEditOperations; ++i) {
		int operation = gen() % 8; // 0,1: place, 2: remove, 3,4,5: connect, 6,7: disconnect
		if (operation <= 1) { // place block
			Orientation orientation(Rotation(gen() % 4), (gen() % 2) == 0);
			BlockType blockType = allowedBlockTypes[gen() % allowedBlockTypes.size()];
			ASSERT_TRUE(blockDataManager.blockExists(blockType));
			Position pos(distPos(gen), distPos(gen));
			bool success = circuit->tryInsertBlock(pos, orientation, blockType);
			if (!success) continue;
			const Block* block = circuit->getBlockContainer().getBlock(pos);
			ASSERT_NE(block, nullptr);
			blockIds.push_back(block->id());
		} else if (operation <= 2) {
			if (blockIds.empty()) continue;
			std::uniform_int_distribution<size_t> distIndex(0, blockIds.size() - 1);
			size_t index = distIndex(gen);
			block_id_t blockId = blockIds[index];
			const Block* block = circuit->getBlockContainer().getBlock(blockId);
			ASSERT_NE(block, nullptr);
			Position pos = block->getPosition();
			ASSERT_TRUE(circuit->tryRemoveBlock(pos));
			std::swap(blockIds[index], blockIds.back());
			blockIds.pop_back();
		} else if (operation <= 5) { // connect
			if (blockIds.empty()) continue;
			block_id_t blockIdA = blockIds[gen() % blockIds.size()];
			block_id_t blockIdB = blockIds[gen() % blockIds.size()];
			const Block* blockA = circuit->getBlockContainer().getBlock(blockIdA);
			const Block* blockB = circuit->getBlockContainer().getBlock(blockIdB);
			ASSERT_NE(blockA, nullptr);
			ASSERT_NE(blockB, nullptr);
			const BlockData* blockDataA = blockDataManager.getBlockData(blockA->type());
			const BlockData* blockDataB = blockDataManager.getBlockData(blockB->type());
			ASSERT_NE(blockDataA, nullptr);
			ASSERT_NE(blockDataB, nullptr);
			std::optional<connection_end_id_t> connA = getRandomConnectionEnd(blockDataA, gen, false);
			std::optional<connection_end_id_t> connB = getRandomConnectionEnd(blockDataB, gen, true);
			if (!connA || !connB) continue;
			// circuit->tryCreateConnection({blockIdA, *connA}, {blockIdB, *connB});
			Position posA = blockA->getConnectionPosition(*connA).value();
			Position posB = blockB->getConnectionPosition(*connB).value();
			circuit->tryCreateConnection(posA, posB);
		} else if (operation <= 7) { // disconnect
			if (blockIds.empty()) continue;
			block_id_t blockIdA = blockIds[gen() % blockIds.size()];
			block_id_t blockIdB = blockIds[gen() % blockIds.size()];
			const Block* blockA = circuit->getBlockContainer().getBlock(blockIdA);
			const Block* blockB = circuit->getBlockContainer().getBlock(blockIdB);
			ASSERT_NE(blockA, nullptr);
			ASSERT_NE(blockB, nullptr);
			const BlockData* blockDataA = blockDataManager.getBlockData(blockA->type());
			const BlockData* blockDataB = blockDataManager.getBlockData(blockB->type());
			ASSERT_NE(blockDataA, nullptr);
			ASSERT_NE(blockDataB, nullptr);
			std::optional<connection_end_id_t> connA = getRandomConnectionEnd(blockDataA, gen, false);
			std::optional<connection_end_id_t> connB = getRandomConnectionEnd(blockDataB, gen, true);
			if (!connA || !connB) continue;
			// circuit->tryRemoveConnection({blockIdA, *connA}, {blockIdB, *connB});
			Position posA = blockA->getConnectionPosition(*connA).value();
			Position posB = blockB->getConnectionPosition(*connB).value();
			circuit->tryRemoveConnection(posA, posB);
		}
	}

	// SAVE CIRCUIT TO FILE
	{
		CircuitFileManager& circuitFileManager = environment.getCircuitFileManager();
		const std::filesystem::path savePath =
			DirectoryManager::getConfigDirectory() / "tmp" / ("BasicFuzzingCircuit_" + testcase_config_to_string(config) + ".cir");
		std::filesystem::create_directories(savePath.parent_path());
		ASSERT_TRUE(circuitFileManager.saveToFile(savePath.string(), circuit->getUUID()));
		logInfo("Saved fuzzing circuit to " + savePath.string(), "BasicFuzzingEvaluatorTest");
	}

	logInfo("Creating reference evaluator", "BasicFuzzingEvaluatorTest");
	evaluator_id_t evalId = environment.getBackend().createEvaluator(circuit->getCircuitId()).value();
	rEval = environment.getBackend().getEvaluator(evalId);
	rEval->setRealistic(runRealistic);
	ASSERT_TRUE(tEval->isPause());
	ASSERT_TRUE(rEval->isPause());
	tEval->resetStates();
	rEval->resetStates();
	std::vector<simulator_id_t> simulatorIdsTest;
	std::vector<simulator_id_t> simulatorIdsRef;
	std::unordered_map<block_id_t, Position> blockIdToPosition;
	std::vector<std::string> ps;
	for (block_id_t blockId : blockIds) {
		const Block* block = circuit->getBlockContainer().getBlock(blockId);
		ASSERT_NE(block, nullptr);
		Position pos = block->getPosition();
		blockIdToPosition[blockId] = pos;
		simulatorIdsTest.push_back(tEval->getBlockSimulatorId(pos));
		simulatorIdsRef.push_back(rEval->getBlockSimulatorId(pos));
		ps.push_back("B " + pos.toString());
		const BlockData* blockData = blockDataManager.getBlockData(block->type());
		ASSERT_NE(blockData, nullptr);
		if (blockData->isDefaultData()) {
			std::variant<simulator_id_t, std::vector<simulator_id_t>> simIdTest = tEval->getPinSimulatorId(pos);
			std::variant<simulator_id_t, std::vector<simulator_id_t>> simIdRef = rEval->getPinSimulatorId(pos);
			if (std::holds_alternative<simulator_id_t>(simIdTest) && std::holds_alternative<simulator_id_t>(simIdRef)) {
				simulatorIdsTest.push_back(std::get<simulator_id_t>(simIdTest));
				simulatorIdsRef.push_back(std::get<simulator_id_t>(simIdRef));
				ps.push_back("P " + pos.toString());
			} else if (std::holds_alternative<std::vector<simulator_id_t>>(simIdTest) && std::holds_alternative<std::vector<simulator_id_t>>(simIdRef)) {
				std::vector<simulator_id_t>& vecTest = std::get<std::vector<simulator_id_t>>(simIdTest);
				std::vector<simulator_id_t>& vecRef = std::get<std::vector<simulator_id_t>>(simIdRef);
				simulatorIdsTest.insert(simulatorIdsTest.end(), vecTest.begin(), vecTest.end());
				simulatorIdsRef.insert(simulatorIdsRef.end(), vecRef.begin(), vecRef.end());
				for (size_t i = 0; i < vecTest.size(); ++i) {
					ps.push_back("P " + pos.toString() + "[" +  std::to_string(i) + "]");
				}
			} else {
				FAIL() << "Mismatched simulator ID types for pin at position " << pos.toString();
			}
		} else {
			const std::unordered_map<connection_end_id_t, BlockData::ConnectionData>& connections = blockData->getConnections();
			for (const auto& [connectionId, connectionData] : connections) {
				if (connectionData.portType == BlockData::ConnectionData::PortType::INPUT) {
					continue;
				}
				std::optional<Position> portPositionOpt = block->getConnectionPosition(connectionId);
				ASSERT_TRUE(portPositionOpt.has_value());
				Position portPosition = portPositionOpt.value();
				std::variant<simulator_id_t, std::vector<simulator_id_t>> simIdTest = tEval->getPinSimulatorId(portPosition);
				std::variant<simulator_id_t, std::vector<simulator_id_t>> simIdRef = rEval->getPinSimulatorId(portPosition);
				if (std::holds_alternative<simulator_id_t>(simIdTest) && std::holds_alternative<simulator_id_t>(simIdRef)) {
					simulatorIdsTest.push_back(std::get<simulator_id_t>(simIdTest));
					simulatorIdsRef.push_back(std::get<simulator_id_t>(simIdRef));
					ps.push_back("P " + portPosition.toString());
				} else if (std::holds_alternative<std::vector<simulator_id_t>>(simIdTest) && std::holds_alternative<std::vector<simulator_id_t>>(simIdRef)) {
					std::vector<simulator_id_t>& vecTest = std::get<std::vector<simulator_id_t>>(simIdTest);
					std::vector<simulator_id_t>& vecRef = std::get<std::vector<simulator_id_t>>(simIdRef);
					ASSERT_EQ(vecTest.size(), vecRef.size()) << "Mismatched simulator ID vector sizes for pin at position " << portPosition.toString();
					simulatorIdsTest.insert(simulatorIdsTest.end(), vecTest.begin(), vecTest.end());
					simulatorIdsRef.insert(simulatorIdsRef.end(), vecRef.begin(), vecRef.end());
					for (size_t i = 0; i < vecTest.size(); ++i) {
						ps.push_back("P " + portPosition.toString() + "[" +  std::to_string(i) + "]");
					}
				} else {
					FAIL() << "Mismatched simulator ID types for pin at position " << portPosition.toString();
				}
			}
		}
	}

	ASSERT_EQ(simulatorIdsTest.size(), simulatorIdsRef.size());
	for (int i = 0; i < numTestOperations; ++i) {
		// set random states
		for (int j = 0; j < numStatesSetPerTest; ++j) {
			block_id_t blockId = blockIds[gen() % blockIds.size()];
			Position pos = blockIdToPosition.at(blockId);
			logic_state_t state = logic_state_t(gen() % 4);
			rEval->setState(pos, state);
			tEval->setState(pos, state);
		}

		// compare states
		std::vector<logic_state_t> statesTest = tEval->getStatesFromSimulatorIds(simulatorIdsTest);
		std::vector<logic_state_t> statesRef = rEval->getStatesFromSimulatorIds(simulatorIdsRef);
		ASSERT_EQ(statesTest.size(), statesRef.size());
		for (size_t k = 0; k < statesTest.size(); ++k) {
			if (statesTest[k] != statesRef[k]) {
				simulator_id_t testSimId = simulatorIdsTest[k];
				simulator_id_t refSimId = simulatorIdsRef[k];
				logInfo("Mismatch at simulator ID index {} (simulator ID test: {}, ref: {}) at p {}", "BasicFuzzingEvaluatorTest", k, testSimId.get(), refSimId.get(), ps.at(k));
				int stepBackAmount = 6;
				for (int m = 0; m < stepBackAmount; ++m) {
					tEval->stepBack();
					rEval->stepBack();
				}
				for (size_t m = 0; m < stepBackAmount; ++m) {
					logInfo("After stepping back {} ticks:", "BasicFuzzingEvaluatorTest", stepBackAmount - m);
					logic_state_t stateTest = tEval->getStateFromSimulatorId(testSimId);
					logic_state_t stateRef = rEval->getStateFromSimulatorId(refSimId);
					logInfo("  Test evaluator state: {}, Reference evaluator state: {}", "BasicFuzzingEvaluatorTest", logicstate_to_string(stateTest), logicstate_to_string(stateRef));
					rEval->stepForward();
					tEval->stepForward();
				}
			}
			ASSERT_EQ(statesTest[k], statesRef[k]) << "Mismatch at simulator ID index " << k << " at p " << ps.at(k);
		}

		// tick
		tEval->tickStep(numTicksBetweenTests);
		rEval->tickStep(numTicksBetweenTests);

		// compare states
		statesTest = tEval->getStatesFromSimulatorIds(simulatorIdsTest);
		statesRef = rEval->getStatesFromSimulatorIds(simulatorIdsRef);
		ASSERT_EQ(statesTest.size(), statesRef.size());
		for (size_t k = 0; k < statesTest.size(); ++k) {
			if (statesTest[k] != statesRef[k]) {
				simulator_id_t testSimId = simulatorIdsTest[k];
				simulator_id_t refSimId = simulatorIdsRef[k];
				logInfo("Mismatch at simulator ID index {} (simulator ID test: {}, ref: {}) at p {}", "BasicFuzzingEvaluatorTest", k, testSimId.get(), refSimId.get(), ps.at(k));
				int stepBackAmount = 6;
				for (int m = 0; m < stepBackAmount; ++m) {
					tEval->stepBack();
					rEval->stepBack();
				}
				for (size_t m = 0; m < stepBackAmount; ++m) {
					logInfo("After stepping back {} ticks:", "BasicFuzzingEvaluatorTest", stepBackAmount - m);
					logic_state_t stateTest = tEval->getStateFromSimulatorId(testSimId);
					logic_state_t stateRef = rEval->getStateFromSimulatorId(refSimId);
					logInfo("  Test evaluator state: {}, Reference evaluator state: {}", "BasicFuzzingEvaluatorTest", logicstate_to_string(stateTest), logicstate_to_string(stateRef));
					rEval->stepForward();
					tEval->stepForward();
				}
			}
			ASSERT_EQ(statesTest[k], statesRef[k]) << "Mismatch at simulator ID index " << k << " at p " << ps.at(k);
		}
	}
}

INSTANTIATE_TEST_SUITE_P(
	RandomSeeds,
	BasicFuzzingEvaluatorTest,
	// ::testing::Range(uint64_t(0), uint64_t(10))
	// ::testing::Values(uint64_t(3))
	testing::ValuesIn(makeTestcases())
	// testing::Combine(
	// 	testing::Values(
	// 		uint64_t(0),
	// 		uint64_t(1),
	// 		uint64_t(2),
	// 		uint64_t(3),
	// 		uint64_t(4)
	// 	),
	// 	testing::Values(false, true),
	// 	testing::Values(
	// 		RunningConfig{5000, 100, 3, 50},
	// 	),
	// 	testing::Values(
	// 		BlockTypesAllowed(
	// 			{
	// 				BlockType::AND,
	// 				BlockType::OR,
	// 				BlockType::XOR,
	// 				BlockType::NAND,
	// 				BlockType::NOR,
	// 				BlockType::XNOR,
	// 				BlockType::BUFFER,
	// 				BlockType::NOT,
	// 				BlockType::JUNCTION,
	// 				BlockType::TRISTATE_BUFFER,
	// 				BlockType::BUTTON,
	// 				BlockType::SWITCH,
	// 				BlockType::CONSTANT_OFF,
	// 				BlockType::CONSTANT_ON,
	// 				BlockType::LIGHT,
	// 			}, {}, {}
	// 		)
	// 	)
	// )
);
