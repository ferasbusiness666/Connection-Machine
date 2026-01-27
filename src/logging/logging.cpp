#include "logging.h"
#include "logger.h"

#include "computerAPI/directoryManager.h"

namespace {
	LogErrorCallback g_logErrorCallback;
	LogWarningCallback g_logWarningCallback;
	LogOutputCallback g_logOutputCallback;
	std::atomic<bool> g_logStdErrEnabled {true};
}

static std::optional<Logger> mainLogger;


Logger& getMainLogger() {
	if (!mainLogger) {
		auto now = std::chrono::system_clock::now();
		std::time_t t = std::chrono::system_clock::to_time_t(now);
		std::tm tm = *std::localtime(&t);
		std::stringstream ss;
		ss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
		std::string logFilePath = (DirectoryManager::getConfigDirectory() / "logs" / ("CM_log_" + ss.str() + ".log")).string();
		std::filesystem::create_directories(DirectoryManager::getConfigDirectory() / "logs");
		mainLogger.emplace(logFilePath);
		std::stringstream ss2;
		ss2 << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
		logInfo("Start Time: {}", "", ss2.str());
		logInfo("Log file path at \"{}\".", "Logger", logFilePath);
	}
	return *mainLogger;
}

void logInfo(const std::string& message, const std::string& subcategory) {
	getMainLogger().log(LogType::Info, message, subcategory, g_logStdErrEnabled.load(std::memory_order_relaxed), g_logOutputCallback);
}

void logWarning(const std::string& message, const std::string& subcategory) {
	getMainLogger().log(LogType::Warning, message, subcategory, g_logStdErrEnabled.load(std::memory_order_relaxed), g_logOutputCallback);
	if (g_logWarningCallback) {
		g_logWarningCallback(message, subcategory);
	}
}

void logError(const std::string& message, const std::string& subcategory) {
	getMainLogger().log(LogType::Error, message, subcategory, g_logStdErrEnabled.load(std::memory_order_relaxed), g_logOutputCallback);
	if (g_logErrorCallback) {
		g_logErrorCallback(message, subcategory);
	}
}

void logFatalError(const std::string& message, const std::string& subcategory) {
	getMainLogger().log(LogType::Fatal, message, subcategory, g_logStdErrEnabled.load(std::memory_order_relaxed), g_logOutputCallback);
}

void setLogErrorCallback(LogErrorCallback callback) { g_logErrorCallback = callback; }

void setLogWarningCallback(LogWarningCallback callback) { g_logWarningCallback = callback; }

void setLogOutputCallback(LogOutputCallback callback) { g_logOutputCallback = callback; }

void setLogStdErrEnabled(bool enabled) { g_logStdErrEnabled.store(enabled, std::memory_order_relaxed); }

std::string getLogContents() { return getMainLogger().getLogContents(); }
