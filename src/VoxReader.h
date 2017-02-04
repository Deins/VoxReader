#pragma once

#include <istream>
#include <vector>

namespace jim {

/**
 * Represents a RGBA color.
 * A packed color is saved into a 4-byte unsigned integer in the format ARGB.
*/
struct RGBA {
	RGBA();
	RGBA(uint32_t color);
	RGBA(uint32_t *color);
	RGBA(uint8_t a, uint8_t r, uint8_t g, uint8_t b);

	void print(std::ostream &s) const;

	uint32_t pack() const;
	void unpack(uint32_t color);

	uint8_t r, g, b, a;
};

/**
 * Represents a single voxel.
*/
class Voxel {
public:
	Voxel(uint8_t x, uint8_t y, uint8_t z, uint8_t colorIndex);
	uint8_t x, y, z, colorIndex;
};

class Model;

/**
 * Contains all models, the palette and materials from a voxel source.
*/
class VoxReader {
public:

	struct Chunk;

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

/**
 * Represents a single model, which has its specific size and a list of voxels.
*/
class Model {
public:

	Model(const VoxReader::Chunk &sizeChunk, const VoxReader::Chunk &xyziChunk);

	uint32_t sizeX, sizeY, sizeZ;
	std::vector<Voxel> voxels;
};

}
