#include <gtest/gtest.h>
#include "loggingTestSetup.h"
#include "logging/logging.h"

namespace {
std::atomic<int> g_errorCount {0};
std::atomic<int> g_warningCount {0};
std::atomic<int> g_expectedErrors {0};
std::atomic<int> g_expectedWarnings {0};

void resetLogCounts() {
	g_errorCount.store(0, std::memory_order_relaxed);
	g_warningCount.store(0, std::memory_order_relaxed);
	g_expectedErrors.store(0, std::memory_order_relaxed);
	g_expectedWarnings.store(0, std::memory_order_relaxed);
}

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
	}

	void OnTestEnd(const ::testing::TestInfo&) override {
		checkLogCounts();
	}
};
} // namespace

namespace logging_test {
void setExpectedLogCounts(int errorCount, int warningCount) {
	g_expectedErrors.store(errorCount, std::memory_order_relaxed);
	g_expectedWarnings.store(warningCount, std::memory_order_relaxed);
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
		setLogErrorCallback({});
		setLogWarningCallback({});
	}
};

::testing::Environment* const logErrorFailureEnvironment =
	::testing::AddGlobalTestEnvironment(new LogErrorFailureEnvironment());
} // namespace
