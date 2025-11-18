#include "randomGens.h"

Position randPos() { return Position(rand() % 100000, rand() % 100000); }
Vector randVec() { return Vector(rand() % 100000, rand() % 100000); }
Size randSize() { return Size(rand() % 100000, rand() % 100000); }