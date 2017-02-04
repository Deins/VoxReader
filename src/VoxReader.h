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

	/**
	 * All exceptions thrown by this library are of this type.
	*/
	class Exception : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	/**
	 * Deallocates the palette if not default.
	*/
	~VoxReader();

	/**
	 * Read the vox-data from the given input stream and stores the read objects.
	 * Discards any objects currently hold.
	*/
	void load(std::istream &s);

	/**
	 * Print the vox-reader's members to the given output stream.
	*/
	void print(std::ostream &s);

	static const uint8_t INVERT_UP = 0x1;
	static const uint8_t FROM_BEHIND = 0x2;
	static const uint8_t SWAP_AXIS = 0x4;

	enum Viewport2d {
		XZ, XY, YZ
	};

	/**
	 * Create a 2D array of voxel pointers in a way they would be seen if looked from a specific camera position.
	 * @param[in] viewport   Model side to be looked at.
	 * @param[in] flags      Optional flags, modifying the order the voxels appear in the returned array. Possible values are:
	 *                       Flag        | Behaviour
	 *                       ------------|-------------------------------------------------------------------------------------
	 *                       INVERT_UP   | Invert voxels along the up axis. The lowest voxel will be seen as the highest voxel.
	 *                       FROM_BEHIND | The layer to be looking at is seen from the back side.
	 *                       SWAP_AXIS   | The up and row axis are swapped.
	 * @param[in] modelIndex Index of model which voxels to be looking at.
	*/
	std::vector<std::vector<Voxel *>> view2d(const Viewport2d viewport, uint8_t flags = 0, uint32_t modelIndex = 0) const;

	/**
	 * If a vox-data does not specifiy a palette, this default palette is used.
	 * The colors are taken from: https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt#L97
	*/
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
