#include <gtest/gtest.h>
#include "loggingTestSetup.h"
#include "logging/logging.h"

namespace {
std::atomic<int> g_errorCount {0};
std::atomic<int> g_warningCount {0};
std::atomic<int> g_expectedErrors {0};
std::atomic<int> g_expectedWarnings {0};
std::atomic<bool> g_logFlushHandledByResultPrinter {false};
std::mutex g_logBufferMutex;
std::vector<std::string> g_logBuffer;

void resetLogCounts() {
	g_errorCount.store(0, std::memory_order_relaxed);
	g_warningCount.store(0, std::memory_order_relaxed);
	g_expectedErrors.store(0, std::memory_order_relaxed);
	g_expectedWarnings.store(0, std::memory_order_relaxed);
}

void resetLogBuffer() {
	std::lock_guard<std::mutex> guard(g_logBufferMutex);
	g_logBuffer.clear();
}

void captureLogLine(const std::string& line) {
	std::lock_guard<std::mutex> guard(g_logBufferMutex);
	g_logBuffer.push_back(line);
}

void flushLogBuffer() {
	std::vector<std::string> lines;
	{
		std::lock_guard<std::mutex> guard(g_logBufferMutex);
		lines.swap(g_logBuffer);
	}
	if (lines.empty()) {
		return;
	}
	for (const auto& line : lines) {
		std::cout << line << "\n";
	}
	std::cout.flush();
}

class LogCaptureInitializer {
public:
	LogCaptureInitializer() {
		setLogStdErrEnabled(false);
		setLogOutputCallback([](const std::string& line) {
			captureLogLine(line);
		});
	}
};

LogCaptureInitializer g_logCaptureInitializer;

void noteError() {
	g_errorCount.fetch_add(1, std::memory_order_relaxed);
}

void noteWarning() {
	g_warningCount.fetch_add(1, std::memory_order_relaxed);
}

void checkLogCounts() {
	const int errors = g_errorCount.load(std::memory_order_relaxed);
	const int warnings = g_warningCount.load(std::memory_order_relaxed);
	const int expectedErrors = g_expectedErrors.load(std::memory_order_relaxed);
	const int expectedWarnings = g_expectedWarnings.load(std::memory_order_relaxed);

	if (errors == expectedErrors && warnings == expectedWarnings) {
		return;
	}

	ADD_FAILURE() << "Expected logError count " << expectedErrors
		<< " and logWarning count " << expectedWarnings
		<< ", got logError " << errors
		<< " and logWarning " << warnings;
}

class LogCountListener : public ::testing::EmptyTestEventListener {
public:
	void OnTestStart(const ::testing::TestInfo&) override {
		resetLogCounts();
		resetLogBuffer();
	}

	void OnTestEnd(const ::testing::TestInfo& testInfo) override {
		checkLogCounts();
		if (!testInfo.result()->Passed()
			&& !g_logFlushHandledByResultPrinter.load(std::memory_order_relaxed)) {
			flushLogBuffer();
		}
	}
};
} // namespace

namespace logging_test {
void setExpectedLogCounts(int errorCount, int warningCount) {
	g_expectedErrors.store(errorCount, std::memory_order_relaxed);
	g_expectedWarnings.store(warningCount, std::memory_order_relaxed);
}

void flushCapturedLogs() {
	flushLogBuffer();
}

void setLogFlushHandledByResultPrinter(bool handled) {
	g_logFlushHandledByResultPrinter.store(handled, std::memory_order_relaxed);
}
} // namespace logging_test

namespace {
class LogErrorFailureEnvironment : public ::testing::Environment {
public:
	void SetUp() override {
		setLogErrorCallback([](const std::string&, const std::string&) {
			noteError();
		});
		setLogWarningCallback([](const std::string&, const std::string&) {
			noteWarning();
		});
		::testing::UnitTest::GetInstance()->listeners().Append(new LogCountListener());
	}

	void TearDown() override {
		setLogOutputCallback({});
		setLogStdErrEnabled(true);
		setLogErrorCallback({});
		setLogWarningCallback({});
	}
};

::testing::Environment* const logErrorFailureEnvironment =
	::testing::AddGlobalTestEnvironment(new LogErrorFailureEnvironment());
} // namespace
