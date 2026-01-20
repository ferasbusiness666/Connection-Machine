#ifndef subcircuitEvalLayer_h
#define subcircuitEvalLayer_h

#include "backend/circuit/circuitDefs.h"
#include "baseEvalLayer.h"

class CircuitManager;
class EvalLayerState;
class Address;

class SubcircuitEvalLayer : public BaseEvalLayer {
public:
	struct SubcircuitData {
		SubcircuitData(circuit_id_t circuitId, const EvalLayerState& outputEvalLayer) : circuitId(circuitId), outputEvalLayer(outputEvalLayer) { }
		circuit_id_t circuitId;
		const EvalLayerState& outputEvalLayer;
		std::unordered_map<eval_gate_id, eval_gate_id> otherSimulatorToThisSimulatorIdMapping;
		std::unordered_map<eval_gate_id, eval_gate_id> thisSimulatorIdMappingToOtherSimulator;
	};
	SubcircuitEvalLayer(EvalLayerState& currentState, Evaluator& evaluator, const CircuitManager& circuitManager) :
		BaseEvalLayer(currentState, circuitManager), evaluator(evaluator) { }
	void run() override final;

	std::vector<std::pair<eval_gate_id, circuit_id_t>> getSubcircuits() const;

	EvalConnectionPoint getMappedAddress(eval_gate_id gateId, const Address& address) const;
	std::vector<EvalConnectionPoint> getReversedMappedConnectionPointsWithAddressMixed(
		const std::vector<EvalConnectionPoint>& connectionPoints,
		eval_gate_id gateId,
		const Address& address
	) const;
	std::vector<std::vector<EvalConnectionPoint>> getReversedMappedConnectionPointGroupsWithAddress(
		const std::vector<std::vector<EvalConnectionPoint>>& connectionPoints,
		eval_gate_id gateId,
		const Address& address
	) const;

	// Only called by EvaluatorInternal
	void processICEdits(circuit_id_t circuitId, const std::vector<connection_end_id_t>& updatedPortIds);
private:
	std::unordered_map<eval_gate_id, SubcircuitData> subcircuits;
	Evaluator& evaluator;
};

#endif /* subcircuitEvalLayer_h */
