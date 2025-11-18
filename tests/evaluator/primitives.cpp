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