#include <gtest/gtest.h>
#include "environment/environment.h"
#include "backend/evaluator/evaluator.h"
#include "backend/blockData/blockDataManager.h"
#include "computerAPI/directoryManager.h"

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
	SharedCircuit busTristate1Circuit = nullptr;
	SharedCircuit busTristate2Circuit = nullptr;
	BlockType BT1;
	BlockType BT2;
	BlockType BUS2;
};

void BusesSimpleEvaluatorTest::SetUp() {
	circuit_id_t circuitId = environment.getBackend().getCircuitManager().createNewCircuit(false);
	circuit = environment.getBackend().getCircuit(circuitId);
	evaluator_id_t evalId = environment.getBackend().createEvaluator(circuitId).value();
	evaluator = environment.getBackend().getEvaluator(evalId);
	CircuitFileManager& circuitFileManager = environment.getCircuitFileManager();
	circuit_id_t busTristate1 = circuitFileManager.loadFromFile((DirectoryManager::getResourceDirectory() / "circuits" / "evaluator" / "bus_tristate_1.cir").string()).at(0);
	busTristate1Circuit = environment.getBackend().getCircuitManager().getCircuit(busTristate1);
	BT1 = busTristate1Circuit->getBlockType();
	circuit_id_t busTristate2 = circuitFileManager.loadFromFile((DirectoryManager::getResourceDirectory() / "circuits" / "evaluator" / "bus_tristate_2.cir").string()).at(0);
	busTristate2Circuit = environment.getBackend().getCircuitManager().getCircuit(busTristate2);
	BT2 = busTristate2Circuit->getBlockType();
	BlockDataManager& blockDataManager = environment.getBackend().getBlockDataManager();
	BUS2 = blockDataManager.getBusBlock(2);
}

void BusesSimpleEvaluatorTest::TearDown() {
	circuit.reset();
	evaluator.reset();
}

TEST_F(BusesSimpleEvaluatorTest, SimpleBus2) {
	BlockDataManager& blockDataManager = environment.getBackend().getBlockDataManager();
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

TEST_F(BusesSimpleEvaluatorTest, BusTristate) {
	BlockDataManager& blockDataManager = environment.getBackend().getBlockDataManager();
	const BlockData* bus2Data = blockDataManager.getBlockData(BUS2);
	ASSERT_NE(bus2Data, nullptr);

	Position switch1Pos(0, 0);
	Position switch2Pos(0, 1);
	Position toggleSwitchPos(0, 2);

	ASSERT_TRUE(circuit->tryInsertBlock(switch1Pos, 0, BlockType::SWITCH));
	ASSERT_TRUE(circuit->tryInsertBlock(switch2Pos, 0, BlockType::SWITCH));
	ASSERT_TRUE(circuit->tryInsertBlock(toggleSwitchPos, 0, BlockType::SWITCH));

	Position busPos(1, 0);
	ASSERT_TRUE(circuit->tryInsertBlock(busPos, 0, BUS2));
	ASSERT_TRUE(circuit->tryCreateConnection(switch1Pos, busPos + *bus2Data->getConnectionVector(0)));
	ASSERT_TRUE(circuit->tryCreateConnection(switch2Pos, busPos + *bus2Data->getConnectionVector(1)));
	Position busOutputPos = busPos + *(bus2Data->getConnectionVector(2));

	Position tristatePos(3, 0);
	ASSERT_TRUE(circuit->tryInsertBlock(tristatePos, 0, BT1));
	ASSERT_TRUE(circuit->tryRemoveBlock(tristatePos));
	ASSERT_TRUE(circuit->tryInsertBlock(tristatePos, 0, BT2));
	ASSERT_TRUE(circuit->tryCreateConnection(busOutputPos, tristatePos));
	ASSERT_TRUE(circuit->tryCreateConnection(toggleSwitchPos, tristatePos + Vector(0, 1)));

	EXPECT_EQ(std::get<std::vector<logic_state_t>>(evaluator->getPinState(tristatePos)), std::vector<logic_state_t>({ Z, Z }));
	evaluator->setState(switch1Pos, H);
	evaluator->tickStep();
	EXPECT_EQ(std::get<std::vector<logic_state_t>>(evaluator->getPinState(tristatePos)), std::vector<logic_state_t>({ Z, Z }));
	evaluator->setState(switch2Pos, H);
	evaluator->tickStep();
	EXPECT_EQ(std::get<std::vector<logic_state_t>>(evaluator->getPinState(tristatePos)), std::vector<logic_state_t>({ Z, Z }));
	evaluator->setState(toggleSwitchPos, H);
	evaluator->tickStep();
	EXPECT_EQ(std::get<std::vector<logic_state_t>>(evaluator->getPinState(tristatePos)), std::vector<logic_state_t>({ H, H }));
	evaluator->setState(switch1Pos, L);
	evaluator->tickStep();
	EXPECT_EQ(std::get<std::vector<logic_state_t>>(evaluator->getPinState(tristatePos)), std::vector<logic_state_t>({ L, H }));
	evaluator->setState(switch2Pos, L);
	evaluator->tickStep();
	EXPECT_EQ(std::get<std::vector<logic_state_t>>(evaluator->getPinState(tristatePos)), std::vector<logic_state_t>({ L, L }));
	evaluator->setState(toggleSwitchPos, L);
	evaluator->tickStep();
	EXPECT_EQ(std::get<std::vector<logic_state_t>>(evaluator->getPinState(tristatePos)), std::vector<logic_state_t>({ Z, Z }));
}
