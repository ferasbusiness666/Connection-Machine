#ifndef simulatorDefs_h
#define simulatorDefs_h

#include "util/id.h"

DECLARE_ID_TYPE(simulator_id_t, unsigned int);
DECLARE_ID_TYPE(simulator_gate_id_t, unsigned int);

typedef std::variant<simulator_gate_id_t, std::vector<simulator_gate_id_t>> SimulatorStateIndexVecVariant;

#endif /* simulatorDefs_h */