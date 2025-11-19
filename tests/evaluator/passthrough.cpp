#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"
#include "computerAPI/directoryManager.h"
#include "computerAPI/directoryManager.h"

class PassthroughEvaluatorTest : public ::testing::Test {
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
	BlockType PT;
};

void PassthroughEvaluatorTest::SetUp() {
	circuit_id_t circuitId = environment.getBackend().createCircuit();
	circuit = environment.getBackend().getCircuit(circuitId);
	evaluator_id_t evalId = environment.getBackend().createEvaluator(circuitId).value();
	evaluator = environment.getBackend().getEvaluator(evalId);
	ASSERT_TRUE(evaluator->isPause());

	CircuitFileManager& circuitFileManager = environment.getCircuitFileManager();
	circuit_id_t passthroughCircuitId = circuitFileManager.loadFromFile((DirectoryManager::getResourceDirectory() / "circuits" / "evaluator" / "passthrough.cir").string()).at(0);
	SharedCircuit passthroughCircuit = environment.getBackend().getCircuitManager().getCircuit(passthroughCircuitId);
	PT = passthroughCircuit->getBlockType();
}

void PassthroughEvaluatorTest::TearDown() {
	circuit.reset();
	evaluator.reset();
}

TEST_F(PassthroughEvaluatorTest, PlacePassthrough) {
	Position blockPos(0, 0);
	ASSERT_TRUE(circuit->tryInsertBlock(blockPos, 0, PT));
	EXPECT_EQ(evaluator->getState(blockPos), L);
}
