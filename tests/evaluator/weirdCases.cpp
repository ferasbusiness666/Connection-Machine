#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/simulator/evalLogicSimulator.h"
#include "computerAPI/directoryManager.h"

class WeirdCasesEvaluatorTest : public ::testing::Test {
protected:
	void SetUp() override;
	void TearDown() override;
	Environment environment { false };
	EvalLogicSimulator* simulator = nullptr;
	SharedCircuit circuit = nullptr;
	BlockType loadCircuit(const std::filesystem::path& path);
	const BlockData* getBlockData(BlockType type);
};

BlockType WeirdCasesEvaluatorTest::loadCircuit(const std::filesystem::path& path) {
	CircuitFileManager& circuitFileManager = environment.getCircuitFileManager();
	circuit_id_t circuitId = circuitFileManager.loadFromFile(path.string()).at(0);
	SharedCircuit circuit = environment.getBackend().getCircuitManager().getCircuit(circuitId);
	return circuit->getBlockType();
}

const BlockData* WeirdCasesEvaluatorTest::getBlockData(BlockType type) {
	const BlockData* blockData = environment.getBackend().getBlockDataManager().getBlockData(type);
	return blockData;
}

void WeirdCasesEvaluatorTest::SetUp() {
	circuit_id_t circuitId = environment.getBackend().getCircuitManager().createNewCircuit(false);
	circuit = environment.getBackend().getCircuit(circuitId);
	simulator_id_t simulatorId = environment.getBackend().createSimulator(circuitId).value();
	simulator = environment.getBackend().getSimulator(simulatorId);
}

void WeirdCasesEvaluatorTest::TearDown() {
	circuit.reset();
	simulator = nullptr;
}

TEST_F(WeirdCasesEvaluatorTest, ConstJunctionAnd) {
	Position constPosition(0, 0);
	Position junctionPosition(1, 0);
	Position andPosition(2, 0);

	ASSERT_TRUE(circuit->tryInsertBlock(constPosition, Rotation::ZERO, BlockType::CONSTANT_ON));
	ASSERT_TRUE(circuit->tryInsertBlock(junctionPosition, Rotation::ZERO, BlockType::JUNCTION));
	ASSERT_TRUE(circuit->tryInsertBlock(andPosition, Rotation::ZERO, BlockType::AND));
	ASSERT_TRUE(circuit->tryCreateConnection(constPosition, junctionPosition));
	ASSERT_TRUE(circuit->tryCreateConnection(junctionPosition, andPosition));
	ASSERT_TRUE(circuit->tryCreateConnection(constPosition, andPosition));

	simulator->tickStep(2);
	EXPECT_EQ(simulator->getState(constPosition), logic_state_t::HIGH);
	EXPECT_EQ(simulator->getState(junctionPosition), logic_state_t::HIGH);
	EXPECT_EQ(simulator->getState(andPosition), logic_state_t::HIGH);

	ASSERT_TRUE(circuit->tryRemoveBlock(junctionPosition));

	simulator->tickStep(2);
	EXPECT_EQ(simulator->getState(constPosition), logic_state_t::HIGH);
	EXPECT_EQ(simulator->getState(andPosition), logic_state_t::HIGH);

	ASSERT_TRUE(circuit->tryRemoveBlock(constPosition));

	simulator->tickStep(2);
	EXPECT_EQ(simulator->getState(andPosition), logic_state_t::LOW);
}

TEST_F(WeirdCasesEvaluatorTest, ButtonBuffer) {
	Position buttonPos(0, 0);
	Position bufferPos(1, 0);

	ASSERT_TRUE(circuit->tryInsertBlock(buttonPos, Rotation::ZERO, BlockType::BUTTON));
	ASSERT_TRUE(circuit->tryInsertBlock(bufferPos, Rotation::ZERO, BlockType::BUFFER));

	simulator->tickStep();
	EXPECT_EQ(simulator->getState(buttonPos), logic_state_t::LOW);
	EXPECT_EQ(simulator->getState(bufferPos), logic_state_t::UNDEFINED);

	ASSERT_TRUE(circuit->tryCreateConnection(buttonPos, bufferPos));
	simulator->tickStep();
	EXPECT_EQ(simulator->getState(buttonPos), logic_state_t::LOW);
	EXPECT_EQ(simulator->getState(bufferPos), logic_state_t::LOW);

	simulator->setState(buttonPos, logic_state_t::HIGH);
	simulator->tickStep();
	EXPECT_EQ(simulator->getState(buttonPos), logic_state_t::HIGH);
	EXPECT_EQ(simulator->getState(bufferPos), logic_state_t::HIGH);

	simulator->setState(buttonPos, logic_state_t::LOW);
	simulator->tickStep();
	EXPECT_EQ(simulator->getState(buttonPos), logic_state_t::LOW);
	EXPECT_EQ(simulator->getState(bufferPos), logic_state_t::LOW);

	ASSERT_TRUE(circuit->tryRemoveConnection(buttonPos, bufferPos));
	simulator->tickStep();
	EXPECT_EQ(simulator->getState(buttonPos), logic_state_t::LOW);
	EXPECT_EQ(simulator->getState(bufferPos), logic_state_t::UNDEFINED);
}

TEST_F(WeirdCasesEvaluatorTest, InitializationBehaviorWithICs) {
	BlockType passthrough = loadCircuit((DirectoryManager::getResourceDirectory() / "circuits" / "evaluator" / "passthrough.cir").string());
	Position xnorPos(0, 0);
	Position icPos(1, 0);
	Position andPos(0, 1);
	ASSERT_TRUE(circuit->tryInsertBlock(xnorPos, Rotation::ZERO, BlockType::XNOR));
	ASSERT_TRUE(circuit->tryInsertBlock(icPos, Rotation::ZERO, passthrough));
	ASSERT_TRUE(circuit->tryInsertBlock(andPos, Rotation::ZERO, BlockType::AND));
	ASSERT_TRUE(circuit->tryCreateConnection(icPos, xnorPos));
	ASSERT_TRUE(circuit->tryCreateConnection(andPos, andPos));
	ASSERT_TRUE(circuit->tryCreateConnection(andPos, icPos));

	EXPECT_EQ(simulator->getState(andPos), logic_state_t::LOW);
	simulator->tickStep(1);
	EXPECT_EQ(simulator->getState(xnorPos), logic_state_t::HIGH);

	simulator->setState(andPos, logic_state_t::HIGH);
	EXPECT_EQ(simulator->getState(andPos), logic_state_t::HIGH);
	simulator->tickStep(1);
	EXPECT_EQ(simulator->getState(xnorPos), logic_state_t::LOW);

	simulator_id_t simulatorId2 = environment.getBackend().createSimulator(circuit->getCircuitId()).value();
	EvalLogicSimulator* simulator2 = environment.getBackend().getSimulator(simulatorId2);
	EXPECT_EQ(simulator2->getState(andPos), logic_state_t::LOW);
	simulator2->tickStep(1);
	EXPECT_EQ(simulator2->getState(xnorPos), logic_state_t::HIGH);

	simulator2->setState(andPos, logic_state_t::HIGH);
	EXPECT_EQ(simulator2->getState(andPos), logic_state_t::HIGH);
	simulator2->tickStep(1);
	EXPECT_EQ(simulator2->getState(xnorPos), logic_state_t::LOW);
}

TEST_F(WeirdCasesEvaluatorTest, PullUpPullDownButWithDifferentConnectionMethod) {
	Position pullUpPos(0, 0);
	Position pullDownPos(1, 0);

	logInfo("Inserting pull-up and pull-down junctions", "WeirdCasesEvaluatorTest::PullUpPullDownButWithDifferentConnectionMethod");

	ASSERT_TRUE(circuit->tryInsertBlock(pullUpPos, Rotation::ZERO, BlockType::JUNCTION_H));
	ASSERT_TRUE(circuit->tryInsertBlock(pullDownPos, Rotation::ZERO, BlockType::JUNCTION_L));

	EXPECT_EQ(simulator->getState(pullUpPos), logic_state_t::HIGH);
	EXPECT_EQ(simulator->getState(pullDownPos), logic_state_t::LOW);

	const Block* pullUpBlock = circuit->getBlockContainer().getBlock(pullUpPos);
	const Block* pullDownBlock = circuit->getBlockContainer().getBlock(pullDownPos);
	ASSERT_NE(pullUpBlock, nullptr);
	ASSERT_NE(pullDownBlock, nullptr);

	logInfo("Creating connection between pull-up and pull-down junctions", "WeirdCasesEvaluatorTest::PullUpPullDownButWithDifferentConnectionMethod");

	ASSERT_TRUE(circuit->tryCreateConnection({ pullUpBlock->id(), connection_end_id_t(0) }, { pullDownBlock->id(), connection_end_id_t(0) }));
	EXPECT_EQ(simulator->getState(pullUpPos), logic_state_t::UNDEFINED);
	EXPECT_EQ(simulator->getState(pullDownPos), logic_state_t::UNDEFINED);

	logInfo("Removing connection between pull-up and pull-down junctions", "WeirdCasesEvaluatorTest::PullUpPullDownButWithDifferentConnectionMethod");

	ASSERT_TRUE(circuit->tryRemoveConnection({ pullDownBlock->id(), connection_end_id_t(0) }, { pullUpBlock->id(), connection_end_id_t(0) }));
	EXPECT_EQ(simulator->getState(pullUpPos), logic_state_t::HIGH);
	EXPECT_EQ(simulator->getState(pullDownPos), logic_state_t::LOW);
}
