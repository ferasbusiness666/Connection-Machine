#include "passThroughEvalLayer.h"
#include "../util/evalLayerState.h"

void PassThroughEvalLayer::run(const EvalLayerState& before, EvalLayerState& after) {
	for (auto iter = before.getRemovedConnectionsBegin(); iter != before.getRemovedConnectionsEnd(); ++iter) {
		after.removeConnection(*iter);
	}
	for (auto iter = before.getRemovedGatesBegin(); iter != before.getRemovedGatesEnd(); ++iter) {
		after.removeGate(*iter);
	}
	for (auto iter = before.getAddedGatesBegin(); iter != before.getAddedGatesEnd(); ++iter) {
		const EvalGate* evalGate = before.getGate(*iter);
		after.addGate(evalGate->gateId, evalGate->type);
	}
	for (auto iter = before.getAddedConnectionsBegin(); iter != before.getAddedConnectionsEnd(); ++iter) {
		after.addConnection(*iter);
	}
	before.getAddedGatesBegin();
	before.getAddedConnectionsBegin();
}
