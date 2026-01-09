#include "passThroughEvalLayer.h"
#include "evalLayerState.h"

void PassThroughEvalLayer::run(const EvalLayerState& currentState, EvalLayerState& nextState) {
	for (auto iter = currentState.getRemovedConnectionsBegin(); iter != currentState.getRemovedConnectionsEnd(); ++iter) {
		nextState.passRemoveConnection(*iter);
	}
	for (auto iter = currentState.getRemovedGatesBegin(); iter != currentState.getRemovedGatesEnd(); ++iter) {
		nextState.passRemoveGate(*iter);
	}
	for (auto iter = currentState.getAddedGatesBegin(); iter != currentState.getAddedGatesEnd(); ++iter) {
		nextState.passAddGate(*iter);
	}
	for (auto iter = currentState.getAddedConnectionsBegin(); iter != currentState.getAddedConnectionsEnd(); ++iter) {
		nextState.passAddConnection(*iter);
	}
}
