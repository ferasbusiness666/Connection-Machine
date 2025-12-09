#include "logsSanitizer.h"

#include <string_view>

std::string sanitizeLogsForPaths(const std::string& logs) {
	// any logs containing "/" or "\" are cropped to just show
	// everything up until the first "]", then resume at the next newline
	std::string sanitizedLogs;
	sanitizedLogs.reserve(logs.size());

	size_t lineStart = 0;
	while (lineStart < logs.size()) {
		size_t newlinePos = logs.find('\n', lineStart);
		if (newlinePos == std::string::npos) {
			newlinePos = logs.size();
		}

		const size_t lineLength = newlinePos - lineStart;
		std::string_view line(logs.data() + lineStart, lineLength);

		const bool containsPath = line.find('/') != std::string_view::npos || line.find('\\') != std::string_view::npos;
		if (containsPath) {
			const size_t bracketPos = line.find(']');
			if (bracketPos != std::string_view::npos) {
				sanitizedLogs.append(line.substr(0, bracketPos + 1));
				sanitizedLogs.append(" OMITTED BECAUSE OF PATH");
			} else {
				sanitizedLogs.append("OMITTED BECAUSE OF PATH");
			}
		} else {
			sanitizedLogs.append(line);
		}

		if (newlinePos < logs.size()) {
			sanitizedLogs.push_back('\n');
			lineStart = newlinePos + 1;
		} else {
			break;
		}
	}

	return sanitizedLogs;
}
