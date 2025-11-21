#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"
#include "backend/blockData/blockDataManager.h"

class BusesSimpleEvaluatorTest : public ::testing::Test {
protected:
	void SetUp() override;
	void TearDown() override;
	Environment environment { false };
	SharedEvaluator evaluator = nullptr;
	SharedCircuit circuit = nullptr;
	logic_state_t L = logic_state_t::LOW;
	logic_state_t H = logic_state_t::HIGH;
	logic_state_t Z = logic_state_t::FLOATING;
	logic_state_t X = logic_state_t::UNDEFINED;
};

void BusesSimpleEvaluatorTest::SetUp() {
	circuit_id_t circuitId = environment.getBackend().getCircuitManager().createNewCircuit(false);
	circuit = environment.getBackend().getCircuit(circuitId);
	evaluator_id_t evalId = environment.getBackend().createEvaluator(circuitId).value();
	evaluator = environment.getBackend().getEvaluator(evalId);
}

void BusesSimpleEvaluatorTest::TearDown() {
	circuit.reset();
	evaluator.reset();
}

TEST_F(BusesSimpleEvaluatorTest, SimpleBus2) {
	BlockDataManager& blockDataManager = environment.getBackend().getBlockDataManager();
	BlockType BUS2 = blockDataManager.getBusBlock(2);
	const BlockData* bus2Data = blockDataManager.getBlockData(BUS2);
	ASSERT_NE(bus2Data, nullptr);
	Position switch1Pos(0, 0);
	Position switch2Pos(0, 1);
	ASSERT_TRUE(circuit->tryInsertBlock(switch1Pos, 0, BlockType::SWITCH));
	ASSERT_TRUE(circuit->tryInsertBlock(switch2Pos, 0, BlockType::SWITCH));

	Position busPos(1, 0);
	ASSERT_TRUE(circuit->tryInsertBlock(busPos, 0, BUS2));

	ASSERT_TRUE(circuit->tryCreateConnection(switch1Pos, busPos + *bus2Data->getConnectionVector(0)));
	ASSERT_TRUE(circuit->tryCreateConnection(switch2Pos, busPos + *bus2Data->getConnectionVector(1)));

	Position busOutputPos = busPos + *(bus2Data->getConnectionVector(2));

	std::variant<logic_state_t, std::vector<logic_state_t>> outputState = evaluator->getPinState(busOutputPos);
	ASSERT_TRUE(std::holds_alternative<std::vector<logic_state_t>>(outputState));
	EXPECT_EQ(std::get<std::vector<logic_state_t>>(outputState), std::vector<logic_state_t>({ L, L }));

	evaluator->setState(switch1Pos, H);

	outputState = evaluator->getPinState(busOutputPos);
	ASSERT_TRUE(std::holds_alternative<std::vector<logic_state_t>>(outputState));
	EXPECT_EQ(std::get<std::vector<logic_state_t>>(outputState), std::vector<logic_state_t>({ H, L }));

	evaluator->setState(switch2Pos, H);

	outputState = evaluator->getPinState(busOutputPos);
	ASSERT_TRUE(std::holds_alternative<std::vector<logic_state_t>>(outputState));
	EXPECT_EQ(std::get<std::vector<logic_state_t>>(outputState), std::vector<logic_state_t>({ H, H }));

	evaluator->setState(switch1Pos, L);

	outputState = evaluator->getPinState(busOutputPos);
	ASSERT_TRUE(std::holds_alternative<std::vector<logic_state_t>>(outputState));
	EXPECT_EQ(std::get<std::vector<logic_state_t>>(outputState), std::vector<logic_state_t>({ L, H }));
}
