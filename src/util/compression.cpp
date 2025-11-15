#include "compression.h"

#include <zlib.h>

Deflater::Deflater(int level) {
    stream = z_stream{};
    int ret = deflateInit(&stream, level);
    if (ret != Z_OK) {
        throw std::runtime_error("deflateInit failed");
    }
}

std::string Deflater::deflateString(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    stream.next_in  = reinterpret_cast<Bytef*>(
        const_cast<char*>(input.data())
    );
    stream.avail_in = static_cast<uInt>(input.size());

    std::string output;
    constexpr std::size_t chunkSize = 16 * 1024;

    int flush = Z_NO_FLUSH;
    int ret = Z_OK;

    do {
        std::size_t oldSize = output.size();
        output.resize(oldSize + chunkSize);

        stream.next_out = reinterpret_cast<Bytef*>(&output[oldSize]);
        stream.avail_out = static_cast<uInt>(chunkSize);

        flush = (stream.avail_in == 0) ? Z_FINISH : Z_NO_FLUSH;

        ret = deflate(&stream, flush);
        if (ret == Z_STREAM_ERROR) {
            throw std::runtime_error("deflate failed");
        }

        std::size_t have = chunkSize - stream.avail_out;
        output.resize(oldSize + have);
    } while (ret != Z_STREAM_END);

    return output;
}
