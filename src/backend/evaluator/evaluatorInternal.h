#ifndef evaluatorInternal_h
#define evaluatorInternal_h

#include "backend/address.h"
#include "evalDefs.h"
#include "layers/layerRunner.h"
#include "backend/circuit/circuitDefs.h"

class CircuitManager;

class EvaluatorInternal {
public:
	EvaluatorInternal(circuit_id_t circuitId, const CircuitManager& circuitManager);
	void startEdit();
	void endEdit();
	void addBlock(Position position, Orientation orientation, BlockType blockType);
	void removeBlock(Position position, Orientation orientation, BlockType blockType);
	void moveBlock(Position curPosition, Orientation curOrientation, Position newPosition, Orientation newOrientation);
	void removeConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition);
	void createConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition);

	const LayerRunner& getLayerRunner() const { return layerRunner; }
	EvalConnectionPoint mapFromTopConnectionPointToBottomConnectionPoint(EvalConnectionPoint topConnectionPoint) const;
	std::optional<std::vector<EvalConnectionPoint>> mapFromBottomConnectionPointToTopConnectionPoint(EvalConnectionPoint bottomConnectionPoint) const;
	std::optional<Address> mapFromTopConnectionPointToAddress(EvalConnectionPoint topConnectionPoint) const;
	std::optional<std::vector<Address>> mapFromBottomConnectionPointToAddress(EvalConnectionPoint bottomConnectionPoint) const;
	EvalConnectionPoint mapFromAddressToTopConnectionPoint(const Address& address) const;
	EvalConnectionPoint mapFromAddressToBottomConnectionPoint(const Address& address) const;
	const std::unordered_map<Position, std::pair<eval_gate_id, Orientation>>& getPositionRemapping() const { return positionRemapping; }
	const std::unordered_map<eval_gate_id, std::pair<Position, Orientation>>& getPositionReverseRemapping() const { return positionReverseRemapping; }

private:
	IdProvider<eval_gate_id> evalGateIdProvider;
	std::unordered_map<Position, std::pair<eval_gate_id, Orientation>> positionRemapping;
	std::unordered_map<eval_gate_id, std::pair<Position, Orientation>> positionReverseRemapping;
	LayerRunner layerRunner;
	const CircuitManager& circuitManager;
	const circuit_id_t circuitId;
};

#endif /* evaluatorInternal_h */
