#include "cliApp.h"

std::thread::id mainThreadId = std::this_thread::get_id();

int main() {
	CliApp app;
	logInfo("Im the CLI app!");
}
