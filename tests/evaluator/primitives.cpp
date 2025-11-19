#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"

class PrimitivesEvaluatorTest : public ::testing::Test {
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

void PrimitivesEvaluatorTest::SetUp() {
    circuit_id_t circuitId = environment.getBackend().createCircuit();
    circuit = environment.getBackend().getCircuit(circuitId);
    evaluator_id_t evalId = environment.getBackend().createEvaluator(circuitId).value();
    evaluator = environment.getBackend().getEvaluator(evalId);
}

void PrimitivesEvaluatorTest::TearDown() {
    circuit.reset();
    evaluator.reset();
}

TEST_F(PrimitivesEvaluatorTest, SingleSwitch) {
    Position switchPos(0, 0);
    ASSERT_TRUE(circuit->tryInsertBlock(switchPos, 0, BlockType::SWITCH));
    ASSERT_EQ(evaluator->getState(switchPos), L);
    evaluator->setState(switchPos, H);
    ASSERT_EQ(evaluator->getState(switchPos), H);
    evaluator->setState(switchPos, L);
    ASSERT_EQ(evaluator->getState(switchPos), L);
}
