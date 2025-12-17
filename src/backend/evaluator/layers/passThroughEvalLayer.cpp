#include "passThroughEvalLayer.h"
#include "../util/evalLayerState.h"

void PassThroughEvalLayer::run(const EvalLayerState& before, EvalLayerState& after) {
	for (auto iter = before.getRemovedConnectionsBegin(); iter != before.getRemovedConnectionsEnd(); ++iter) { // need to add mapping throught eval or give layers state.
		// *iter
		// after.addConnection();
	}
	for (auto iter = before.getAddedGatesBegin(); iter != before.getAddedGatesEnd(); ++iter) {
		// const EvalGate* evalGate = before.getGate(*iter);
		// after.addGate(iter->get());
	}
	before.getAddedGatesBegin();
	before.getAddedConnectionsBegin();
}
