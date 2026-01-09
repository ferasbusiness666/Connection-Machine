#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"

class NoEditEvaluatorTest : public ::testing::Test {
protected:
	void SetUp() override;
	void TearDown() override;
	Environment environment {false};
	SharedEvaluator evaluator = nullptr;
	SharedCircuit circuit = nullptr;
};

void NoEditEvaluatorTest::SetUp() {
	circuit_id_t circuitId = environment.getBackend().getCircuitManager().createNewCircuit(false);
	circuit = environment.getBackend().getCircuit(circuitId);
	evaluator_id_t evalId = environment.getBackend().createEvaluator(circuitId).value();
	evaluator = environment.getBackend().getEvaluator(evalId);
}

void NoEditEvaluatorTest::TearDown() {
	circuit.reset();
	evaluator.reset();
}

TEST_F(NoEditEvaluatorTest, PauseUnpause) {
	evaluator->getEvalLogicSimulator().setPause(true);
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isPause());
	EXPECT_DOUBLE_EQ(evaluator->getEvalLogicSimulator().getRealTickrate(), 0.0);
	evaluator->getEvalLogicSimulator().setPause(false);
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().isPause());
	// wait up to 200 ms for the background thread to update the tickrate
	auto start = std::chrono::steady_clock::now();
	bool tickPositive = false;
	while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(200)) {
		if (evaluator->getEvalLogicSimulator().getRealTickrate() > 0.0) { tickPositive = true; break; }
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	EXPECT_TRUE(tickPositive);
}

TEST_F(NoEditEvaluatorTest, TickStep) {
	evaluator->getEvalLogicSimulator().setPause(true);
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isPause());
	evaluator->getEvalLogicSimulator().tickStep();
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isPause());
	EXPECT_DOUBLE_EQ(evaluator->getEvalLogicSimulator().getRealTickrate(), 0.0);
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().isViewingReplay());
}

TEST_F(NoEditEvaluatorTest, RealisticMode) {
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().isRealistic());
	evaluator->getEvalLogicSimulator().setRealistic(true);
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().isRealistic());
	evaluator->getEvalLogicSimulator().setRealistic(false);
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().isRealistic());
}

TEST_F(NoEditEvaluatorTest, TickrateSetting) {
	evaluator->getEvalLogicSimulator().setTickrate(20.0);
	EXPECT_DOUBLE_EQ(evaluator->getEvalLogicSimulator().getTickrate(), 20.0);
	evaluator->getEvalLogicSimulator().setTickrate(100.0);
	EXPECT_DOUBLE_EQ(evaluator->getEvalLogicSimulator().getTickrate(), 100.0);
}

TEST_F(NoEditEvaluatorTest, UseTickrateSetting) {
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().getUseTickrate());
	evaluator->getEvalLogicSimulator().setUseTickrate(false);
	EXPECT_FALSE(evaluator->getEvalLogicSimulator().getUseTickrate());
	evaluator->getEvalLogicSimulator().setUseTickrate(true);
	EXPECT_TRUE(evaluator->getEvalLogicSimulator().getUseTickrate());
}

TEST_F(NoEditEvaluatorTest, EvalName) {
	std::string expectedName = "Eval " + std::to_string(evaluator->getEvaluatorId()) + " (" + circuit->getCircuitNameNumber() + ")";
	EXPECT_EQ(evaluator->getEvaluatorName(), expectedName);
}
