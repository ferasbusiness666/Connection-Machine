#ifndef layerRunner_h
#define layerRunner_h

#include "backend/circuit/circuitDefs.h"
#include "../evalDefs.h"

class CircuitManager;
class BlockDataManager;
class EvalLayerState;
class BaseEvalLayer;
class Address;

typedef std::vector<std::vector<EvalConnectionPoint>> VecVecEvalConnectionPoint;
typedef std::vector<EvalConnectionPoint> VecEvalConnectionPoint;

class LayerRunner {
public:
	LayerRunner(IdProvider<eval_gate_id>& evalGateIdProvider, Evaluator& evaluator, const CircuitManager& circuitManager);
	~LayerRunner();
	void runAll();
	void resetEdits();
	EvalLayerState& getInputLayer();
	const EvalLayerState& getInputLayer() const;
	const EvalLayerState& getOutputLayer() const;
	const EvalLayerState& getOutputLayerForOtherEvals() const;

	std::vector<std::pair<eval_gate_id, circuit_id_t>> getSubcircuits() const;

	EvalConnectionPoint getMappedAddress(eval_gate_id gateId, const Address& address) const;
	EvalConnectionPoint getMappedAddressForOtherEvals(eval_gate_id gateId, const Address& address) const;
	VecEvalConnectionPoint getReversedMappedConnectionPointWithAddress(EvalConnectionPoint evalConnectionPoint, eval_gate_id gateId, const Address& address) const;
	VecVecEvalConnectionPoint getReversedMappedConnectionPointsWithAddress(const VecEvalConnectionPoint& evalConnectionPoints, eval_gate_id gateId, const Address& address) const;
	VecEvalConnectionPoint getReversedMappedConnectionPointsWithAddressMixedForOtherEvals(VecEvalConnectionPoint evalConnectionPoints, eval_gate_id gateId, const Address& address) const;
	VecVecEvalConnectionPoint getReversedMappedConnectionPointGroupsWithAddress(VecVecEvalConnectionPoint evalConnectionPoints, eval_gate_id gateId, const Address& address) const;
	VecVecEvalConnectionPoint getReversedMappedConnectionPointGroupsWithAddressForOtherEvals(VecVecEvalConnectionPoint evalConnectionPoints, eval_gate_id gateId, const Address& address) const;
	EvalConnectionPoint getMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const;
	EvalConnectionPoint getMappedEvalConnectionPointForOtherEvals(EvalConnectionPoint evalConnectionPoint) const;
	VecEvalConnectionPoint getReversedMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const;
	void getReversedMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint, VecEvalConnectionPoint& outputVector) const;
	void getReversedMappedEvalConnectionPointForOtherEvals(EvalConnectionPoint evalConnectionPoint, VecEvalConnectionPoint& outputVector) const;
private:
	std::unique_ptr<EvalLayerState> evalTopLayerState;
	std::vector<std::unique_ptr<BaseEvalLayer>> layers;
};

#endif /* layerRunner_h */