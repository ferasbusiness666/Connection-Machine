#ifndef evalDefs_h
#define evalDefs_h

#include "backend/blockData/blockData.h"

DECLARE_ID_TYPE(evaluator_id_t, unsigned int);
DECLARE_ID_TYPE(eval_circuit_id_t, unsigned int);
DECLARE_ID_TYPE(eval_gate_id, unsigned int);
DECLARE_ID_TYPE(simulator_id_t, unsigned int);

typedef unsigned int EvalGateType;

inline EvalGateType getEvalGateType(BlockType blockType) { return (EvalGateType)blockType; }
inline BlockType getBlockType(EvalGateType evalGateType) { return (BlockType)evalGateType; }

struct SimulatorMappingUpdate {
	SimulatorMappingUpdate(Position position, const std::variant<simulator_id_t, std::vector<simulator_id_t>>& simulatorIds) : position(position), simulatorIds(simulatorIds) {}
	SimulatorMappingUpdate(Position position, std::optional<virtual_connection_id_t> virtualConnectionId, const std::variant<simulator_id_t, std::vector<simulator_id_t>>& simulatorIds) :
		position(position), virtualConnectionId(virtualConnectionId), simulatorIds(simulatorIds) {}
	Position position;
	std::optional<virtual_connection_id_t> virtualConnectionId = std::nullopt;
	std::variant<simulator_id_t, std::vector<simulator_id_t>> simulatorIds;
};

typedef std::function<void(const std::vector<SimulatorMappingUpdate>&)> SimulatorMappingUpdateListenerFunction;

struct SimulatorMappingUpdateListener {
	eval_circuit_id_t evalCircuitId;
	std::function<void(const std::vector<SimulatorMappingUpdate>&)> callback;
};

struct EvalConnectionPoint {
	EvalConnectionPoint(eval_gate_id gateId, connection_end_id_t connectionEndId) : gateId(gateId), connectionEndId(connectionEndId) { }
	static inline EvalConnectionPoint null() { return EvalConnectionPoint(0, 0); }
	bool isNull() const { return gateId == 0; }

	eval_gate_id gateId;
	connection_end_id_t connectionEndId;

	bool operator==(const EvalConnectionPoint other) const { return gateId == other.gateId && connectionEndId == other.connectionEndId; }
};

template <>
struct std::hash<EvalConnectionPoint> {
	inline std::size_t operator()(EvalConnectionPoint evalConnectionPoint) const noexcept {
		std::size_t h1 = std::hash<eval_gate_id>{}(evalConnectionPoint.gateId);
		std::size_t h2 = std::hash<connection_end_id_t>{}(evalConnectionPoint.connectionEndId);
		std::size_t seed = h1 + 0x9e3779b9;
		return h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
};

struct EvalConnection {
	EvalConnection(eval_gate_id gateAId, connection_end_id_t gateAConnectionId, eval_gate_id gateBId, connection_end_id_t gateBConnectionId) :
		connectionPointA(gateAId, gateAConnectionId), connectionPointB(gateBId, gateBConnectionId) { }
	EvalConnection(EvalConnectionPoint connectionPointA, EvalConnectionPoint connectionPointB) :
		connectionPointA(connectionPointA), connectionPointB(connectionPointB) { }

	EvalConnectionPoint connectionPointA;
	EvalConnectionPoint connectionPointB;

	bool operator==(const EvalConnection other) const { return connectionPointA == other.connectionPointA && connectionPointB == other.connectionPointB; }
};

template <>
struct std::hash<EvalConnection> {
	inline std::size_t operator()(EvalConnection evalConnection) const noexcept {
		std::size_t h1 = std::hash<EvalConnectionPoint>{}(evalConnection.connectionPointA);
		std::size_t h2 = std::hash<EvalConnectionPoint>{}(evalConnection.connectionPointB);
		std::size_t seed = h1 + 0x9e3779b9;
		return h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
};

class Evaluator;
typedef std::shared_ptr<class Evaluator> SharedEvaluator;

#endif /* evalDefs_h */
