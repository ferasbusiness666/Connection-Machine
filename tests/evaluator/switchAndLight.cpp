#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"

class SwitchAndLightEvaluatorTest : public ::testing::Test {
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

void SwitchAndLightEvaluatorTest::SetUp() {
	circuit_id_t circuitId = environment.getBackend().getCircuitManager().createNewCircuit(false);
	circuit = environment.getBackend().getCircuit(circuitId);
	evaluator_id_t evalId = environment.getBackend().createEvaluator(circuitId).value();
	evaluator = environment.getBackend().getEvaluator(evalId);
	ASSERT_TRUE(evaluator->isPause());
}

void SwitchAndLightEvaluatorTest::TearDown() {
	circuit.reset();
	evaluator.reset();
}

TEST_F(SwitchAndLightEvaluatorTest, SingleSwitch) {
	Position switchPos(0, 0);
	ASSERT_TRUE(circuit->tryInsertBlock(switchPos, 0, BlockType::SWITCH));
	EXPECT_EQ(evaluator->getState(switchPos), L);
	evaluator->setState(switchPos, H);
	EXPECT_EQ(evaluator->getState(switchPos), H);
	evaluator->setState(switchPos, L);
	EXPECT_EQ(evaluator->getState(switchPos), L);
}

TEST_F(SwitchAndLightEvaluatorTest, InteractFail) {
	Position nothingPos(10, 10);
	EXPECT_EQ(evaluator->getState(nothingPos), X);
	evaluator->setState(nothingPos, H); // should log an error
	EXPECT_EQ(evaluator->getState(nothingPos), X);
}

TEST_F(SwitchAndLightEvaluatorTest, SwitchAndLight) {
	Position switchPos(0, 0);
	Position lightPos(1, 0);
	ASSERT_TRUE(circuit->tryInsertBlock(switchPos, 0, BlockType::SWITCH));
	ASSERT_TRUE(circuit->tryInsertBlock(lightPos, 0, BlockType::LIGHT));

	// switch and light not connected yet
	EXPECT_EQ(evaluator->getState(switchPos), L);
	EXPECT_EQ(evaluator->getState(lightPos), Z);
	evaluator->setState(switchPos, H);
	EXPECT_EQ(evaluator->getState(switchPos), H);
	EXPECT_EQ(evaluator->getState(lightPos), Z);
	evaluator->setState(switchPos, L);
	EXPECT_EQ(evaluator->getState(switchPos), L);
	EXPECT_EQ(evaluator->getState(lightPos), Z);

	// connect switch to light
	ASSERT_TRUE(circuit->tryCreateConnection(switchPos, lightPos));

	EXPECT_EQ(evaluator->getState(switchPos), L);
	EXPECT_EQ(evaluator->getState(lightPos), L);
	evaluator->setState(switchPos, H);
	EXPECT_EQ(evaluator->getState(switchPos), H);
	EXPECT_EQ(evaluator->getState(lightPos), H);
	evaluator->setState(switchPos, L);
	EXPECT_EQ(evaluator->getState(switchPos), L);
	EXPECT_EQ(evaluator->getState(lightPos), L);

	// disconnect switch from light
	ASSERT_TRUE(circuit->tryRemoveConnection(switchPos, lightPos));
	EXPECT_EQ(evaluator->getState(switchPos), L);
	EXPECT_EQ(evaluator->getState(lightPos), Z);
	evaluator->setState(switchPos, H);
	EXPECT_EQ(evaluator->getState(switchPos), H);
	EXPECT_EQ(evaluator->getState(lightPos), Z);
	evaluator->setState(switchPos, L);
	EXPECT_EQ(evaluator->getState(switchPos), L);
	EXPECT_EQ(evaluator->getState(lightPos), Z);
}

TEST_F(SwitchAndLightEvaluatorTest, TwoSwitchesAndLight) {
	Position switch1Pos(0, 0);
	Position switch2Pos(1, 0);
	Position lightPos(2, 0);
	ASSERT_TRUE(circuit->tryInsertBlock(switch1Pos, 0, BlockType::SWITCH));
	ASSERT_TRUE(circuit->tryInsertBlock(switch2Pos, 0, BlockType::SWITCH));
	ASSERT_TRUE(circuit->tryInsertBlock(lightPos, 0, BlockType::LIGHT));

	EXPECT_EQ(evaluator->getState(switch1Pos), L);
	EXPECT_EQ(evaluator->getState(switch2Pos), L);
	EXPECT_EQ(evaluator->getState(lightPos), Z);

	// connect switches to light
	ASSERT_TRUE(circuit->tryCreateConnection(switch1Pos, lightPos));
	ASSERT_TRUE(circuit->tryCreateConnection(switch2Pos, lightPos));

	EXPECT_EQ(evaluator->getState(switch1Pos), L);
	EXPECT_EQ(evaluator->getState(switch2Pos), L);
	EXPECT_EQ(evaluator->getState(lightPos), L);

	evaluator->setState(switch1Pos, H);
	EXPECT_EQ(evaluator->getState(switch1Pos), H);
	EXPECT_EQ(evaluator->getState(switch2Pos), L);
	EXPECT_EQ(evaluator->getState(lightPos), X); // contention

	evaluator->setState(switch2Pos, H);
	EXPECT_EQ(evaluator->getState(switch1Pos), H);
	EXPECT_EQ(evaluator->getState(switch2Pos), H);
	EXPECT_EQ(evaluator->getState(lightPos), H);

	evaluator->setState(switch1Pos, L);
	EXPECT_EQ(evaluator->getState(switch1Pos), L);
	EXPECT_EQ(evaluator->getState(switch2Pos), H);
	EXPECT_EQ(evaluator->getState(lightPos), X); // contention

	evaluator->setState(switch2Pos, L);
	EXPECT_EQ(evaluator->getState(switch1Pos), L);
	EXPECT_EQ(evaluator->getState(switch2Pos), L);
	EXPECT_EQ(evaluator->getState(lightPos), L);
}
