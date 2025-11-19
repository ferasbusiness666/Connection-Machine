#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"

class ReplaysEvaluatorTest : public ::testing::Test {
protected:
	void SetUp() override;
	void TearDown() override;
	Environment environment {false};
	SharedEvaluator evaluator = nullptr;
	SharedCircuit circuit = nullptr;
	logic_state_t L = logic_state_t::LOW;
	logic_state_t H = logic_state_t::HIGH;
	logic_state_t Z = logic_state_t::FLOATING;
	logic_state_t X = logic_state_t::UNDEFINED;
};

void ReplaysEvaluatorTest::SetUp() {
	circuit_id_t circuitId = environment.getBackend().createCircuit();
	circuit = environment.getBackend().getCircuit(circuitId);
	evaluator_id_t evalId = environment.getBackend().createEvaluator(circuitId).value();
	evaluator = environment.getBackend().getEvaluator(evalId);
	evaluator->setPause(true);
	ASSERT_TRUE(evaluator->isPause());

	// clear any initial simulation that may have occurred between evaluator creation and pausing
	circuit->tryInsertBlock(Position(0, 0), 0, BlockType::SWITCH);
	circuit->clear(true);
	ASSERT_FALSE(evaluator->isViewingReplay());
}

void ReplaysEvaluatorTest::TearDown() {
	circuit.reset();
	evaluator.reset();
}

TEST_F(ReplaysEvaluatorTest, StepBackEmptyHistory) {
	EXPECT_FALSE(evaluator->stepBack());
	EXPECT_FALSE(evaluator->isViewingReplay());
}

TEST_F(ReplaysEvaluatorTest, SkipBackEmptyHistory) {
	EXPECT_FALSE(evaluator->skipBack());
	EXPECT_FALSE(evaluator->isViewingReplay());
}

TEST_F(ReplaysEvaluatorTest, StepForward) {
	evaluator->stepForward();
	EXPECT_FALSE(evaluator->isViewingReplay());
	EXPECT_TRUE(evaluator->isPause());
}

TEST_F(ReplaysEvaluatorTest, StepForwardThenBack) {
	evaluator->stepForward();
	EXPECT_FALSE(evaluator->isViewingReplay());
	EXPECT_TRUE(evaluator->isPause());
	EXPECT_TRUE(evaluator->stepBack());
	EXPECT_TRUE(evaluator->isViewingReplay());
	EXPECT_TRUE(evaluator->isPause());
}

TEST_F(ReplaysEvaluatorTest, SkipForwardWithoutReplay) {
	EXPECT_FALSE(evaluator->skipForward());
	EXPECT_FALSE(evaluator->isViewingReplay());
	EXPECT_TRUE(evaluator->isPause());
}

TEST_F(ReplaysEvaluatorTest, StepForwardALotThenBack) {
	for (int i = 0; i < 20; i++) {
		evaluator->stepForward();
	}
	EXPECT_FALSE(evaluator->isViewingReplay());
	EXPECT_TRUE(evaluator->isPause());
	for (int i = 0; i < 20; i++) {
		EXPECT_TRUE(evaluator->stepBack());
		EXPECT_TRUE(evaluator->isViewingReplay());
	}
	EXPECT_TRUE(evaluator->isPause());

	EXPECT_FALSE(evaluator->stepBack());
	EXPECT_TRUE(evaluator->isViewingReplay());
	EXPECT_TRUE(evaluator->isPause());

	EXPECT_TRUE(evaluator->skipForward());
	EXPECT_FALSE(evaluator->isViewingReplay());
	EXPECT_TRUE(evaluator->isPause());
}
