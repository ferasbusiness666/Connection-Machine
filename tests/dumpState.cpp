#include <gtest/gtest.h>
#include "environment/environment.h"
#include <nlohmann/json.hpp>

class DumpStateTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
    Environment environment { false };
    SharedCircuit circuit = nullptr;
};

void DumpStateTest::SetUp() {
    circuit_id_t circuitId = environment.getBackend().getCircuitManager().createNewCircuit(true);
    circuit = environment.getBackend().getCircuit(circuitId);
}

void DumpStateTest::TearDown() {
    circuit.reset();
}

TEST_F(DumpStateTest, DumpEnvironmentState) {
    // TODO: make the test actually good
    nlohmann::json stateJson = environment.dumpState();
    EXPECT_TRUE(stateJson.contains("backend"));
    EXPECT_TRUE(stateJson.contains("circuitFileManager"));
    EXPECT_TRUE(stateJson.contains("fileListener"));
}