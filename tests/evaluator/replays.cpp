#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"

class ReplaysEvaluatorTest : public ::testing::Test {
protected:
	void SetUp() override;
	void TearDown() override;
	Environment environment {false};
	Evaluator* evaluator = nullptr;
	SharedCircuit circuit = nullptr;
	logic_state_t L = logic_state_t::LOW;
	logic_state_t H = logic_state_t::HIGH;
	logic_state_t Z = logic_state_t::FLOATING;
	logic_state_t X = logic_state_t::UNDEFINED;
};

void ReplaysEvaluatorTest::SetUp() {
	circuit_id_t circuitId = environment.getBackend().getCircuitManager().createNewCircuit(false);
	circuit = environment.getBackend().getCircuit(circuitId);
	evaluator_id_t evalId = environment.getBackend().createEvaluator(circuitId).value();
	evaluator = environment.getBackend().getEvaluator(evalId);
	ASSERT_TRUE(evaluator->getEvalLogicSimulator().isPause());
	ASSERT_FALSE(evaluator->getEvalLogicSimulator().isViewingReplay());
}

void ReplaysEvaluatorTest::TearDown() {
	circuit.reset();
	evaluator = nullptr;
}

TEST_F(ReplaysEvaluatorTest, StepBackEmptyHistory) {
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().stepBack());
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().isViewingReplay());
}

TEST_F(ReplaysEvaluatorTest, SkipBackEmptyHistory) {
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().skipBack());
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().isViewingReplay());
}

TEST_F(ReplaysEvaluatorTest, StepForward) {
	evaluator->getEvalLogicSimulator().stepForward();
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().isViewingReplay());
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isPause());
}

TEST_F(ReplaysEvaluatorTest, StepForwardThenBack) {
	evaluator->getEvalLogicSimulator().stepForward();
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().isViewingReplay());
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isPause());
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().stepBack());
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isViewingReplay());
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isPause());
}

TEST_F(ReplaysEvaluatorTest, SkipForwardWithoutReplay) {
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().skipForward());
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().isViewingReplay());
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isPause());
}

TEST_F(ReplaysEvaluatorTest, StepForwardALotThenBack) {
	for (int i = 0; i < 20; i++) {
		evaluator->getEvalLogicSimulator().stepForward();
	}
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().isViewingReplay());
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isPause());
	for (int i = 0; i < 20; i++) {
		EXPECT_TRUE(evaluator->getEvalLogicSimulator().stepBack());
		EXPECT_TRUE(evaluator->getEvalLogicSimulator().isViewingReplay());
	}
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isPause());

	EXPECT_FALSE(evaluator->getEvalLogicSimulator().stepBack());
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isViewingReplay());
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isPause());

	EXPECT_TRUE(evaluator->getEvalLogicSimulator().skipForward());
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().isViewingReplay());
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isPause());
}
