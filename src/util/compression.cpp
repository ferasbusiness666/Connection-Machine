#include "compression.h"

#include <brotli/encode.h>

std::string compressString(const std::string& input) {
	if (input.empty()) {
		return {};
	}

	const size_t maxSize = BrotliEncoderMaxCompressedSize(input.size());
	if (maxSize == 0) {
		throw std::runtime_error("BrotliEncoderMaxCompressedSize failed");
	}

	std::string output(maxSize, '\0');
	size_t encodedSize = maxSize;

	const auto result = BrotliEncoderCompress(
		BROTLI_DEFAULT_QUALITY,
		BROTLI_DEFAULT_WINDOW,
		BROTLI_DEFAULT_MODE,
		input.size(),
		reinterpret_cast<const uint8_t*>(input.data()),
		&encodedSize,
		reinterpret_cast<uint8_t*>(output.data())
	);

	if (result == BROTLI_FALSE) {
		throw std::runtime_error("Brotli compression failed");
	}

	output.resize(encodedSize);
	return output;
}
