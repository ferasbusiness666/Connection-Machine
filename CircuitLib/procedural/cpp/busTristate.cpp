#include "cm.h"

extern "C" {

const char* UUID = "0662c20a-c47f-4586-bafc-f582587b230b";
const char* name = "Bus_Tristate_Buffer";
const char* defaultParameters = "(\"size\": 1)";

bool generateCircuit() {
	int size = getParameter("size");

	BlockType blockType_Tristate_Buffer = getPrimitiveType("TRISTATE_BUFFER");
	BlockType blockType_Bus = getBusBlock(size);
	BlockType blockType_SWITCH = getPrimitiveType("SWITCH");

	setSize(1, 2);

	block_id_t inBus = createBlockAtPosition(0, 0, 6, blockType_Bus);
	block_id_t outBus = createBlockAtPosition(5, 0, 0, blockType_Bus);
	block_id_t enable = createBlockAtPosition(0, -1, 0, blockType_SWITCH);
	setConnectionPortBitWidth(addConnectionInputNamed(0, 0, inBus, size, "Out"), size);
	setConnectionPortBitWidth(addConnectionOutputNamed(0, 0, outBus, size, "In"), size);
	addConnectionInputNamed(0, 1, enable, 0, "Enable");

	block_id_t C;
	for (int i = 0; i < size; i++) {
		block_id_t buffer = createBlockAtPosition(3, i*2, 0, blockType_Tristate_Buffer);
		createConnection(buffer, 2, outBus, i);
		createConnection(inBus, i, buffer, 0);
		createConnection(enable, 0, buffer, 1);
	}

	return true;
}

}