#include "logger.h"

#ifdef TRACY_PROFILER
#include <tracy/Tracy.hpp>
#endif

#define ANSI_INFO "\033[1;37m"
#define ANSI_WARNING "\033[1;33m"
#define ANSI_ERROR "\033[1;31m"
#define ANSI_FATAL "\033[1;4;41;97m"
#define ANSI_TAIL "\033[0m"

Logger::Logger(const std::filesystem::path& outputFile) : outputFile(outputFile), outputFileStream(outputFile) { }

void Logger::log(
	LogType type,
	const std::string& message,
	const std::string& subcategory,
	bool emitToStdErr,
	const std::function<void(const std::string&)>& outputCallback) {
	std::lock_guard<std::mutex> guard(loggingMutex);

	std::string categorySuffix;
	if (subcategory != "") {
		categorySuffix = " - " + subcategory;
	}

	const char* ansiPrefix = "";
	const char* categoryLabel = "";
	switch (type) {
	// output to stderr
	case LogType::Info:
		ansiPrefix = ANSI_INFO;
		categoryLabel = "Info";
		break;
	case LogType::Warning:
		ansiPrefix = ANSI_WARNING;
		categoryLabel = "Warning";
		break;
	case LogType::Error:
		ansiPrefix = ANSI_ERROR;
		categoryLabel = "ERROR";
		break;
	case LogType::Fatal:
		ansiPrefix = ANSI_FATAL;
		categoryLabel = "FATAL";
		break;
	}

	std::string categoryText = std::string(categoryLabel) + categorySuffix;
	std::string consoleLine = "[" + std::string(ansiPrefix) + categoryText + ANSI_TAIL + "] " + message;
	if (emitToStdErr) {
		std::cerr << consoleLine << "\n";
		std::cerr.flush();
	}
	if (outputCallback) {
		outputCallback(consoleLine);
	}

#ifdef TRACY_PROFILER
	// output to tracy
	std::string msg = "[" + categoryText + "] " + message;
	TracyMessage(msg.c_str(), msg.size());
#endif

	// output to file
	outputFileStream << "[" << categoryText << "] " << message << "\n";
	outputFileStream.flush();
}

std::string Logger::getLogContents() const {
	std::lock_guard<std::mutex> guard(loggingMutex);

	std::ifstream input(outputFile, std::ios::binary);
	if (!input.is_open()) {
		return "";
	}

	std::ostringstream buffer;
	buffer << input.rdbuf();
	return buffer.str();
}
