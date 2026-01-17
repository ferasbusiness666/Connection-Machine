#include "baseEvalLayer.h"

#include "evalLayerState.h"

BaseEvalLayer::BaseEvalLayer(EvalLayerState& currentState) : currentState(currentState), nextState(currentState.getOrMakeNextLayerState()) {}
