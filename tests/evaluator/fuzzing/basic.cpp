#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"
#include "backend/container/block/blockDefs.h"
#include "backend/blockData/blockDataManager.h"
#include "computerAPI/directoryManager.h"

class BasicFuzzingEvaluatorTest : public ::testing::TestWithParam<uint64_t> {
protected:
	void SetUp() override;
	void TearDown() override;
	std::mt19937_64 gen;
	Environment environment {false};
	SharedCircuit circuit = nullptr;
	SharedEvaluator tEval = nullptr; // testing evaluator
	SharedEvaluator rEval = nullptr; // reference evaluator
};

void BasicFuzzingEvaluatorTest::SetUp() {
	gen.seed(GetParam());
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
	bool runRealistic = gen() % 2 == 0;
	tEval->setRealistic(runRealistic);
	BlockDataManager& blockDataManager = environment.getBackend().getBlockDataManager();
	std::uniform_int_distribution<int> distPos(-20, 20);
	std::vector<block_id_t> blockIds;
	int numEditOperations = 5000;
	int numTestOperations = 100;
	int numTicksBetweenTests = 3;
	int numStatesSetPerTest = 50;
	for (int i = 0; i < numEditOperations; ++i) {
		int operation = gen() % 8; // 0,1: place, 2: remove, 3,4,5: connect, 6,7: disconnect
		if (operation <= 1) { // place block
			Orientation orientation(Rotation(gen() % 4), (gen() % 2) == 0);
			BlockType blockType = BlockType((gen() % blockDataManager.maxBlockId()) + 1);
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
			circuit->tryCreateConnection({blockIdA, *connA}, {blockIdB, *connB});
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
			circuit->tryRemoveConnection({blockIdA, *connA}, {blockIdB, *connB});
		}
	}

	// SAVE CIRCUIT TO FILE
	{
		CircuitFileManager& circuitFileManager = environment.getCircuitFileManager();
		const std::filesystem::path savePath =
			DirectoryManager::getConfigDirectory() / "tmp" / ("BasicFuzzingCircuit_" + std::to_string(GetParam()) + ".cir");
		std::filesystem::create_directories(savePath.parent_path());
		ASSERT_TRUE(circuitFileManager.saveToFile(savePath.string(), circuit->getUUID()));
		logInfo("Saved fuzzing circuit to " + savePath.string(), "BasicFuzzingEvaluatorTest");
	}

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
		logInfo("Starting test operation {} of {}", "BasicFuzzingEvaluatorTest", i + 1, numTestOperations);
		// set random states
		for (int j = 0; j < numStatesSetPerTest; ++j) {
			block_id_t blockId = blockIds[gen() % blockIds.size()];
			Position pos = blockIdToPosition.at(blockId);
			logic_state_t state = logic_state_t(gen() % 4);
			logInfo("Setting state at block ID {} position {} to {}", "BasicFuzzingEvaluatorTest", blockId, pos.toString(), logicstate_to_string(state));
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
	::testing::Values(uint64_t(3))
);
