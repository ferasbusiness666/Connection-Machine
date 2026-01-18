#ifndef subcircuitEvalLayer_h
#define subcircuitEvalLayer_h

#include "backend/circuit/circuitDefs.h"
#include "baseEvalLayer.h"

class CircuitManager;
class EvalLayerState;
class Address;

class SubcircuitEvalLayer : public BaseEvalLayer {
	struct SubcircuitData {
		SubcircuitData(circuit_id_t circuitId, const EvalLayerState& outputEvalLayer) : circuitId(circuitId), outputEvalLayer(outputEvalLayer) {}
		circuit_id_t circuitId;
		const EvalLayerState& outputEvalLayer;
		std::unordered_map<eval_gate_id, eval_gate_id> otherSimulatorToThisSimulatorIdMapping;
		std::unordered_map<eval_gate_id, eval_gate_id> thisSimulatorIdMappingToOtherSimulator;
	};
public:
	SubcircuitEvalLayer(EvalLayerState& currentState, Evaluator& evaluator, const CircuitManager& circuitManager) :
		BaseEvalLayer(currentState), evaluator(evaluator), circuitManager(circuitManager) {}
	void run() override final;

	EvalConnectionPoint getMappedAddress(eval_gate_id gateId, const Address& address) const;
	std::vector<EvalConnectionPoint> getReversedMappedConnectionPointsWithAddressMixed(const std::vector<EvalConnectionPoint>& connectionPoints, eval_gate_id gateId, const Address& address) const;
	std::vector<std::vector<EvalConnectionPoint>> getReversedMappedConnectionPointGroupsWithAddress(const std::vector<std::vector<EvalConnectionPoint>>& connectionPoints, eval_gate_id gateId, const Address& address) const;

	// Only called by EvaluatorInternal
	void processICEdits(circuit_id_t circuitId, const std::vector<connection_end_id_t>& updatedPortIds);
private:
	std::unordered_map<eval_gate_id, SubcircuitData> subcircuits;
	Evaluator& evaluator;
	const CircuitManager& circuitManager;
};

#endif /* subcircuitEvalLayer_h */
