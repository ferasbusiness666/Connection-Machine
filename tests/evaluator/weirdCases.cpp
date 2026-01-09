#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"
#include "computerAPI/directoryManager.h"

class WeirdCasesEvaluatorTest : public ::testing::Test {
protected:
	void SetUp() override;
	void TearDown() override;
	Environment environment { false };
	SharedEvaluator evaluator = nullptr;
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
	evaluator_id_t evalId = environment.getBackend().createEvaluator(circuitId).value();
	evaluator = environment.getBackend().getEvaluator(evalId);
}

void WeirdCasesEvaluatorTest::TearDown() {
	circuit.reset();
	evaluator.reset();
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

	evaluator->getEvalLogicSimulator().tickStep(2);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(constPosition), logic_state_t::HIGH);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(junctionPosition), logic_state_t::HIGH);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(andPosition), logic_state_t::HIGH);

	ASSERT_TRUE(circuit->tryRemoveBlock(junctionPosition));

	evaluator->getEvalLogicSimulator().tickStep(2);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(constPosition), logic_state_t::HIGH);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(andPosition), logic_state_t::HIGH);

	ASSERT_TRUE(circuit->tryRemoveBlock(constPosition));

	evaluator->getEvalLogicSimulator().tickStep(2);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(andPosition), logic_state_t::LOW);
}

TEST_F(WeirdCasesEvaluatorTest, ButtonBuffer) {
	Position buttonPos(0, 0);
	Position bufferPos(1, 0);

	ASSERT_TRUE(circuit->tryInsertBlock(buttonPos, Rotation::ZERO, BlockType::BUTTON));
	ASSERT_TRUE(circuit->tryInsertBlock(bufferPos, Rotation::ZERO, BlockType::BUFFER));

	evaluator->getEvalLogicSimulator().tickStep();
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(buttonPos), logic_state_t::LOW);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(bufferPos), logic_state_t::UNDEFINED);

	ASSERT_TRUE(circuit->tryCreateConnection(buttonPos, bufferPos));
	evaluator->getEvalLogicSimulator().tickStep();
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(buttonPos), logic_state_t::LOW);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(bufferPos), logic_state_t::LOW);

	evaluator->getEvalLogicSimulator().setState(buttonPos, logic_state_t::HIGH);
	evaluator->getEvalLogicSimulator().tickStep();
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(buttonPos), logic_state_t::HIGH);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(bufferPos), logic_state_t::HIGH);

	evaluator->getEvalLogicSimulator().setState(buttonPos, logic_state_t::LOW);
	evaluator->getEvalLogicSimulator().tickStep();
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(buttonPos), logic_state_t::LOW);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(bufferPos), logic_state_t::LOW);

	ASSERT_TRUE(circuit->tryRemoveConnection(buttonPos, bufferPos));
	evaluator->getEvalLogicSimulator().tickStep();
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(buttonPos), logic_state_t::LOW);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(bufferPos), logic_state_t::UNDEFINED);
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

	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(andPos), logic_state_t::LOW);
	evaluator->getEvalLogicSimulator().tickStep(1);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(xnorPos), logic_state_t::HIGH);

	evaluator->getEvalLogicSimulator().setState(andPos, logic_state_t::HIGH);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(andPos), logic_state_t::HIGH);
	evaluator->getEvalLogicSimulator().tickStep(1);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(xnorPos), logic_state_t::LOW);

	evaluator_id_t evalId2 = environment.getBackend().createEvaluator(circuit->getCircuitId()).value();
	SharedEvaluator evaluator2 = environment.getBackend().getEvaluator(evalId2);
	EXPECT_EQ(evaluator2->getEvalLogicSimulator().getState(andPos), logic_state_t::LOW);
	evaluator2->getEvalLogicSimulator().tickStep(1);
	EXPECT_EQ(evaluator2->getEvalLogicSimulator().getState(xnorPos), logic_state_t::HIGH);

	evaluator2->getEvalLogicSimulator().setState(andPos, logic_state_t::HIGH);
	EXPECT_EQ(evaluator2->getEvalLogicSimulator().getState(andPos), logic_state_t::HIGH);
	evaluator2->getEvalLogicSimulator().tickStep(1);
	EXPECT_EQ(evaluator2->getEvalLogicSimulator().getState(xnorPos), logic_state_t::LOW);
}

TEST_F(WeirdCasesEvaluatorTest, PullUpPullDownButWithDifferentConnectionMethod) {
	Position pullUpPos(0, 0);
	Position pullDownPos(1, 0);

	logInfo("Inserting pull-up and pull-down junctions", "WeirdCasesEvaluatorTest::PullUpPullDownButWithDifferentConnectionMethod");

	ASSERT_TRUE(circuit->tryInsertBlock(pullUpPos, Rotation::ZERO, BlockType::JUNCTION_H));
	ASSERT_TRUE(circuit->tryInsertBlock(pullDownPos, Rotation::ZERO, BlockType::JUNCTION_L));

	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(pullUpPos), logic_state_t::HIGH);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(pullDownPos), logic_state_t::LOW);

	const Block* pullUpBlock = circuit->getBlockContainer().getBlock(pullUpPos);
	const Block* pullDownBlock = circuit->getBlockContainer().getBlock(pullDownPos);
	ASSERT_NE(pullUpBlock, nullptr);
	ASSERT_NE(pullDownBlock, nullptr);

	logInfo("Creating connection between pull-up and pull-down junctions", "WeirdCasesEvaluatorTest::PullUpPullDownButWithDifferentConnectionMethod");

	ASSERT_TRUE(circuit->tryCreateConnection({ pullUpBlock->id(), connection_end_id_t(0) }, { pullDownBlock->id(), connection_end_id_t(0) }));
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(pullUpPos), logic_state_t::UNDEFINED);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(pullDownPos), logic_state_t::UNDEFINED);

	logInfo("Removing connection between pull-up and pull-down junctions", "WeirdCasesEvaluatorTest::PullUpPullDownButWithDifferentConnectionMethod");

	ASSERT_TRUE(circuit->tryRemoveConnection({ pullDownBlock->id(), connection_end_id_t(0) }, { pullUpBlock->id(), connection_end_id_t(0) }));
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(pullUpPos), logic_state_t::HIGH);
	EXPECT_EQ(evaluator->getEvalLogicSimulator().getState(pullDownPos), logic_state_t::LOW);
}
