#ifndef compression_h
#define compression_h

class Deflater {
public:
	explicit Deflater(int level);
	Deflater(const Deflater&) = delete;
	Deflater& operator=(const Deflater&) = delete;

	~Deflater() {
		deflateEnd(&stream);
	}

	std::string deflateString(const std::string& input);

private:
	z_stream stream;
};

#endif /* compression_h */
