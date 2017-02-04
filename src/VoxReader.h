#pragma once

#include <istream>
#include <vector>

namespace jim {

struct Chunk {
	Chunk(std::istream &s, int *byteOffset = nullptr);

	void print(int indent, std::ostream &s) const;

	char id[5];
	std::vector<uint8_t> content;
	std::vector<Chunk> children;
};

struct RGBA {
	RGBA();
	RGBA(uint32_t color);
	RGBA(uint32_t *color);
	RGBA(uint8_t a, uint8_t r, uint8_t g, uint8_t b);

	uint8_t r, g, b, a;

	void print(std::ostream &s) const;

	uint32_t zip() const;
	void unzip(uint32_t color);
};

class Voxel {
public:
	Voxel(uint8_t x, uint8_t y, uint8_t z, uint8_t colorIndex);
	uint8_t x, y, z, colorIndex;
};

class Model {
public:

	Model(const Chunk &sizeChunk, const Chunk &xyziChunk);

	uint32_t sizeX, sizeY, sizeZ;
	std::vector<Voxel> voxels;
};

class VoxReader {
public:

	class Exception : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	void load(std::istream &s);

	void print(std::ostream &s);

	static std::vector<RGBA> DEFAULT_PALETTE;

private:

	std::vector<Model> models;
	std::vector<RGBA> *palette = &DEFAULT_PALETTE;

};

}
