#ifndef loggingTestSetup_h
#define loggingTestSetup_h

namespace logging_test {

void setExpectedLogCounts(int errorCount, int warningCount);
void flushCapturedLogs();
void setLogFlushHandledByResultPrinter(bool handled);

} // namespace logging_test

#endif /* loggingTestSetup_h */
