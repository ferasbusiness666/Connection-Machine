#ifndef layerRunner_h
#define layerRunner_h

#include "../evalDefs.h"

class CircuitManager;
class BlockDataManager;
class EvalLayerState;
class BaseEvalLayer;
class Address;

class LayerRunner {
public:
	LayerRunner(IdProvider<eval_gate_id>& evalGateIdProvider, Evaluator& evaluator, const CircuitManager& circuitManager);
	~LayerRunner();
	void runAll();
	void resetEdits();
	EvalLayerState& getInputLayer();
	const EvalLayerState& getInputLayer() const;
	const EvalLayerState& getOutputLayer() const;
	EvalConnectionPoint getMappedAddress(eval_gate_id gateId, const Address& address) const;
	std::vector<EvalConnectionPoint> getReversedMappedConnectionPointWithAddress(EvalConnectionPoint evalConnectionPoint, eval_gate_id gateId, const Address& address) const;
	std::vector<std::vector<EvalConnectionPoint>> getReversedMappedConnectionPointsWithAddress(const std::vector<EvalConnectionPoint>& evalConnectionPoints, eval_gate_id gateId, const Address& address) const;
	std::vector<EvalConnectionPoint> getReversedMappedConnectionPointsWithAddressMixed(std::vector<EvalConnectionPoint> evalConnectionPoints, eval_gate_id gateId, const Address& address) const;
	std::vector<std::vector<EvalConnectionPoint>> getReversedMappedConnectionPointGroupsWithAddress(std::vector<std::vector<EvalConnectionPoint>> evalConnectionPoints, eval_gate_id gateId, const Address& address) const;
	EvalConnectionPoint getMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const;
	std::vector<EvalConnectionPoint> getReversedMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint) const;
	void getReversedMappedEvalConnectionPoint(EvalConnectionPoint evalConnectionPoint, std::vector<EvalConnectionPoint>& outputVector) const;
private:
	std::unique_ptr<EvalLayerState> evalTopLayerState;
	std::vector<std::unique_ptr<BaseEvalLayer>> layers;
};

#endif /* layerRunner_h */