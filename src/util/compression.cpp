#include "compression.h"

#include <brotli/encode.h>

std::optional<std::string> compressString(const std::string& input) {
	if (input.empty()) {
		return {};
	}

	const size_t maxSize = BrotliEncoderMaxCompressedSize(input.size());
	if (maxSize == 0) {
		logError("BrotliEncoderMaxCompressedSize returned 0");
		return std::nullopt;
	}

	std::string output(maxSize, '\0');
	size_t encodedSize = maxSize;

	constexpr int compressionQuality = 5;
	constexpr int compressionWindow = 18;
	const auto result = BrotliEncoderCompress(
		compressionQuality,
		compressionWindow,
		BROTLI_DEFAULT_MODE,
		input.size(),
		reinterpret_cast<const uint8_t*>(input.data()),
		&encodedSize,
		reinterpret_cast<uint8_t*>(output.data())
	);

	if (result == BROTLI_FALSE) {
		logError("BrotliEncoderCompress failed");
		return std::nullopt;
	}

	output.resize(encodedSize);
	return output;
}
