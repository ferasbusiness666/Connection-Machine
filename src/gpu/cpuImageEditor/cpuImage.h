#ifndef cpuImageEditor_h
#define cpuImageEditor_h

#include "gpu/freetype/font.h"
#include "util/vec2.h"

class CpuImage {
public:
	struct Pixel {
		Pixel(uint8_t r=0, uint8_t g=0, uint8_t b=0, uint8_t a=255) : r(r), g(g), b(b), a(a) {}
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};

	CpuImage(Vec2Int size, Pixel color = Pixel()) : img(size.x * size.y, color), imgSize(size) {}
	const uint8_t* getData() const { return (uint8_t*)img.data(); }

	Vec2Int getSize() const { return imgSize; }

	void addRect(Vec2Int pos, Vec2Int size, Pixel color, bool mix = true);
	void blitAlphaTexture(const uint8_t* texture, Vec2Int bufferSize, Vec2Int bufferTexturePos, Vec2Int size, Vec2Int pos, Pixel color, Rotation rotation = Rotation::ZERO);
	Vec2Int blitChar(std::shared_ptr<Font> font, char c, Vec2Int pos, Pixel color, Rotation rotation = Rotation::ZERO);
	void writeString(std::shared_ptr<Font> font, const std::string&, Vec2Int pos, Pixel color, Rotation rotation = Rotation::ZERO);
	void writeStringInArea(std::shared_ptr<Font> font, const std::string& text, Vec2Int pos, Vec2Int areaSize, Pixel color, Rotation rotation = Rotation::ZERO, bool centerV = false, bool centerH= false, unsigned int startingFontSize = 0);

private:
	unsigned int getBufferPos(Vec2Int pos) { return pos.x + pos.y * imgSize.x; }

	std::vector<Pixel> img;
	Vec2Int imgSize;
};

#endif /* cpuImageEditor */
