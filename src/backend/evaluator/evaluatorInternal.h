#ifndef evaluatorInternal_h
#define evaluatorInternal_h

#include "backend/address.h"
#include "evalDefs.h"
#include "layers/layerRunner.h"
#include "backend/circuit/circuitDefs.h"

class CircuitManager;

class EvaluatorInternal {
public:
	struct InternalPointData {
		InternalPointData(std::optional<EvalConnectionPoint> connectionPoint, BlockData::ConnectionData::PortType portType, unsigned int bitWith) :
			connectionPoint(connectionPoint), portType(portType), bitWith(bitWith) {}
		InternalPointData(BlockData::ConnectionData::PortType portType, unsigned int bitWith) : portType(portType), bitWith(bitWith) {}
		std::optional<EvalConnectionPoint> connectionPoint;
		BlockData::ConnectionData::PortType portType;
		unsigned int bitWith;
	};
	EvaluatorInternal(const Circuit& circuit, const CircuitManager& circuitManager, DataUpdateEventManager::DataUpdateEventReceiver& receiver);
	void startEdit();
	void endEdit();
	void addBlock(Position position, Orientation orientation, BlockType blockType);
	void removeBlock(Position position, Orientation orientation, BlockType blockType);
	void moveBlock(Position curPosition, Orientation curOrientation, Position newPosition, Orientation newOrientation);
	void removeConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition);
	void createConnection(Position outputBlockPosition, Position outputPosition, Position inputBlockPosition, Position inputPosition);

	const LayerRunner& getLayerRunner() const { return layerRunner; }
	EvalConnectionPoint mapFromTopConnectionPointToBottomConnectionPoint(EvalConnectionPoint topConnectionPoint) const;
	std::vector<EvalConnectionPoint> mapFromBottomConnectionPointToTopConnectionPoint(EvalConnectionPoint bottomConnectionPoint) const;
	std::optional<Address> mapFromTopConnectionPointToAddress(EvalConnectionPoint topConnectionPoint) const;
	std::optional<std::pair<Address, Address>> mapFromTopConnectionPointToPointAndBlockAddress(EvalConnectionPoint bottomConnectionPoint) const;
	std::vector<Address> mapFromBottomConnectionPointToAddress(EvalConnectionPoint bottomConnectionPoint) const;
	std::vector<std::pair<Address, Address>> mapFromBottomConnectionPointToAddressAndBlockAddress(EvalConnectionPoint bottomConnectionPoint) const;
	EvalConnectionPoint mapFromAddressToTopConnectionPoint(const Address& address) const;
	EvalConnectionPoint mapFromAddressToBottomConnectionPoint(const Address& address) const;
	const std::unordered_map<Position, std::pair<eval_gate_id, Orientation>>& getPositionRemapping() const { return positionRemapping; }
	const std::unordered_map<eval_gate_id, std::pair<Position, Orientation>>& getPositionReverseRemapping() const { return positionReverseRemapping; }
	const std::unordered_map<connection_end_id_t, InternalPointData>& getPortToInternalPointMapping() const { return portToInternalPointMapping; }

private:
	// std::set<Position> positionsTo; nothing to do
	IdProvider<eval_gate_id> evalGateIdProvider;
	std::unordered_map<Position, std::pair<eval_gate_id, Orientation>> positionRemapping;
	std::unordered_map<eval_gate_id, std::pair<Position, Orientation>> positionReverseRemapping;
	std::unordered_map<connection_end_id_t, InternalPointData> portToInternalPointMapping;
	LayerRunner layerRunner;
	const CircuitManager& circuitManager;
	const Circuit& circuit;
};

#endif /* evaluatorInternal_h */
