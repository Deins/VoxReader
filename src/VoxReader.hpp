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
	using Dictionary = std::vector<std::pair<std::string, std::string>>; // may be unsorted!
	class SceneGraph;

	/**
	 * Represents a layer metadata
	*/
	class Layer {
	public:
		Dictionary attributes; // see extended voxl spec for list of possible values
	};

	/**
	 * Represents a material from extended format (this represents 'MATL' chunk and not 'MATT' one)
	*/
	class MaterialEx {
	public:
		Dictionary properties; // see extended voxl spec for list of possible values
	};

	/**
	 * Represents a scene graph for magica voxel scene.
	 * 	T : Transform Node
	 *	G : Group Node
	 *	S : Shape Node
	 *
	 *		 T
	 *		 |
	 *		 G
	 *		/ \
	 *	   T   T
	 *	   |   |
	 *	   G   S
	 *	  / \
	 *	 T   T
	 *	 |   |
	 *	 S   S
	 *
	*/
	class SceneGraph {
	public:
		friend class VoxReader;

		typedef int32_t NodeId;

		/**
		 * Represents model with attributes how to display it
		*/
		struct Model {
			uint32_t modelId; // index into VoxReader::models
			Dictionary attributes;
		};

		/**
		 * Represent scene graph node
		*/
		class Node {
		public:
			enum Type : uint8_t {
				TRANSFORM,
				GROUP,
				SHAPE
			};

			Type type;
			Dictionary attributes;
		};

		/**
		 * Represents transformation to be applied
		*/
		class TransformNode : public Node {
		public:

			inline TransformNode() { type = TRANSFORM; }

			NodeId childNodeId;
			int32_t layerId;
			std::vector<Dictionary> frame_attributes;
		};

		/**
		 * Represents a group
		*/
		class GroupNode : public Node {
		public:
			inline GroupNode() { type = GROUP; }

			std::vector<NodeId> childNodeIds;
		};

		/**
		 * Represents one or multiple shapes
		*/
		class ShapeNode : public Node {
		public:
			inline ShapeNode() { type = SHAPE; }

			std::vector<Model> models;
		};

		/**
		 * Returns node by its index or NULL when scene graph is no available or node can't be found
		*/
		const Node* GetNode(NodeId id) const;

		/**
		 * Returns root node of scene graph or NULL when scene graph is no available
		*/
		inline const Node* GetRoot() const { return GetNode(0); }

	protected:
		// TODO: pass end of chunk ptr and check bounds 
		void readTransformNode(const uint8_t* ptr);
		void readGroupNode(const uint8_t* ptr);
		void readShapeNode(const uint8_t* ptr);
		template <typename NODE> NODE& addNode(NodeId id);

	private:

		/**
		 * When scene graph is available stores all its nodes. Tree root should be at index 0.
		*/
		std::vector<std::unique_ptr<Node>> nodes; // 0 is the root node; if no scene graph found will be 0

	};

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

		//private:

		std::vector<Model> models;
		std::vector<RGBA> *palette = &DEFAULT_PALETTE;
		SceneGraph sceneGraph; // no need for heap allocation, but because of nested classes there is no other option.
		std::vector<Layer> layers;
		std::vector<MaterialEx> materials;

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

#ifdef JIM_VOXREADER_IMPLEMENTATION

#include <iostream>
#include <iomanip>

namespace jim {

	//////////////////////////////////////////////////////////////////////////////
	// UTILITIES
	//////////////////////////////////////////////////////////////////////////////

	static int readInt(std::istream &s) {
		uint32_t integer;
		s.read(reinterpret_cast<char *>(&integer), 4);
		return integer;
	}

	static int readInt(const uint8_t *bytearray) {
		union {
			uint32_t integer;
			uint8_t bytes[4];
		} ret;
		ret.bytes[0] = bytearray[0];
		ret.bytes[1] = bytearray[1];
		ret.bytes[2] = bytearray[2];
		ret.bytes[3] = bytearray[3];
		return ret.integer;
	}

	//////////////////////////////////////////////////////////////////////////////
	// CHUNK
	//////////////////////////////////////////////////////////////////////////////

	/**
	* Chunks are generated while loading voxel data and are discarded
	* when loading is finished.
	* Due to the reason that this class is not supposed to be used by the user,
	* it is defined privately.
	*/
	struct VoxReader::Chunk {
		Chunk(std::istream &s, int *byteOffset = nullptr);

		void print(int indent, std::ostream &s) const;

		char id[5];
		std::vector<uint8_t> content;
		std::vector<Chunk> children;
	};

	VoxReader::Chunk::Chunk(std::istream &s, int *byteOffset) {
		int rootByte = 0;
		if (byteOffset == nullptr) {
			byteOffset = &rootByte;
		}

		s.read(id, 4);
		id[4] = '\0';

		int contentSize = readInt(s);
		int childrenSize = readInt(s);

		*byteOffset += 4 + 4 + 4;

		int startByte = *byteOffset;

		*byteOffset += contentSize;

		// Read content:
		content.resize(contentSize);
		s.read(reinterpret_cast<char *>(content.data()), content.size());

		// Read children:
		while (*byteOffset - startByte < childrenSize) {
			children.push_back(Chunk(s, byteOffset));
		}

		*byteOffset += childrenSize;
	}

	void VoxReader::Chunk::print(int indent, std::ostream &s) const {
		static const int INDENT_LENGTH = 4;
		std::string tab(indent * INDENT_LENGTH, ' ');
		s << tab.c_str() << '[' << id << ']' << std::endl;
		s << tab.c_str() << "  Content size: " << std::hex << std::uppercase << std::showbase << content.size() << " bytes" << std::endl;

		// Print content:
		s << tab.c_str() << "  Content:";
		for (size_t i = 0; i < content.size(); i++) {
			if (0 == i % 16) s << std::endl << tab.c_str() << "    ";
			s << std::hex << std::noshowbase << std::setw(2) << std::setfill('0') << (unsigned)content[i] << ' ';
		}
		s << std::endl;

		// Print children:
		s << tab.c_str() << "  Children: " << std::hex << std::uppercase << std::showbase << children.size() << std::endl;
		for (const auto child : children) {
			child.print(indent + 1, s);
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	// EXTEDNED FORMAT UTILITIES
	//////////////////////////////////////////////////////////////////////////////
	std::string readString(const uint8_t** input) {
		int32_t size = readInt(*input);
		*input += 4;
		std::string str(*input, *input + size);
		*input += size;
		return str;
	}

	std::vector < std::pair<std::string, std::string>> readDictionary(const uint8_t** ptr) {
		int entries = readInt(*ptr);
		*ptr += 4;
		std::vector < std::pair<std::string, std::string>> dictionary;
		dictionary.reserve(entries);
		for (int i = 0; i < entries; ++i) {
			std::string key = readString(ptr);
			std::string value = readString(ptr);
			dictionary.emplace_back(key, value);
		}
		return dictionary;
	}

	//////////////////////////////////////////////////////////////////////////////
	// VOX-READER
	//////////////////////////////////////////////////////////////////////////////

	template<typename... C>
	static inline std::vector<RGBA> makePalette(C... colors) {
		return std::vector<RGBA>{ (RGBA(colors))... };
	}

	std::vector<RGBA> VoxReader::DEFAULT_PALETTE = makePalette(
		0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
		0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
		0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
		0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
		0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
		0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
		0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
		0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
		0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
		0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
		0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
		0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
		0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
		0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
		0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
		0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
	);

	VoxReader::~VoxReader() {
		if ((palette != nullptr) && (palette != &DEFAULT_PALETTE)) {
			delete palette;
		}
	}

	void VoxReader::load(std::istream &s) {

		if (!s) {
			throw Exception("Cannot read from stream");
		}

		// Reset current state:
		models.clear();
		if (palette != &DEFAULT_PALETTE) {
			delete palette;
		}

		// Check for the magic string "VOX ":
		char magic[4];
		s.read(magic, 4);
		if (!strcmp(magic, "VOX ")) {
			throw Exception("Magic string 'VOX ' is missing");
		}

		// Check version of VOX file:
		if (readInt(s) != 150) {
			throw Exception("Version is not 150");
		}

		// Read main chunk:
		Chunk main(s);

		// Create models based on chunk tree:
		uint32_t modelCount;
		auto iter = main.children.begin();
		Chunk &pack = main.children[0];

		if (!strcmp(pack.id, "PACK")) {
			modelCount = readInt(&pack.content[0]);
			iter++;
		}
		else {
			// Has no 'PACK' spec => 1 model:
			modelCount = 1;
		}

		while (iter != main.children.end()) {

			// Model:
			if (!strcmp(iter->id, "SIZE")) {
				auto sizeChunkIter = iter++;
				auto xyziChunkIter = iter++;
				models.push_back(Model(*sizeChunkIter, *xyziChunkIter));
			}

			// Palette:
			else if (!strcmp(iter->id, "RGBA")) {
				auto rgbaChunkIter = iter++;
				palette = new std::vector<RGBA>;
				palette->resize(256);
				memcpy(&(*palette)[0], &rgbaChunkIter->content[0], 4 * 256);
			}

			// Node:
			else if (iter->id[0] == 'n') {

				// Scene transform node:
				if (!strcmp(iter->id, "nTRN")) {
					sceneGraph.readTransformNode(iter++->content.data());
				}

				// Scene group node:
				else if (!strcmp(iter->id, "nGRP")) {
					sceneGraph.readGroupNode(iter++->content.data());
				}

				// Scene group node:
				else if (!strcmp(iter->id, "nSHP")) {
					sceneGraph.readShapeNode(iter->content.data());
				}
				else throw Exception("Unknown node!");

				++iter; // chunk processed
			}

			// Layer:
			else if (!strcmp(iter->id, "LAYR")) {
				const uint8_t* ptr = &(iter++)->content[0];
				int32 layerId = readInt(ptr);
				ptr += 4;
				if (layerId <= layers.size()) {
					layers.resize(layerId + 1);
				}
				layers[layerId].attributes = readDictionary(&ptr);
			}

			// Material (extended):
			else if (!strcmp(iter->id, "MATL")) {
				const uint8_t* ptr = &(iter++)->content[0];
				int32 matId = readInt(ptr);
				ptr += 4;
				if (matId <= materials.size()) {
					materials.resize(matId + 1);
				}
				materials[matId].properties = readDictionary(&ptr);
			}

			// throw to avoid infinite loop
			else {
				//throw("Unknown chunk!");
				++iter;
			}
		}
	}

	void VoxReader::print(std::ostream & s) {
		s << "VOXEL-OBJECT:" << std::endl;

		s << "Num models: " << models.size() << std::endl;
		for (const auto model : models) {
			s << std::noshowbase << "Model:  size(" << model.sizeX << ',' << model.sizeY << ',' << model.sizeZ << ")" << std::endl;
			for (const auto voxel : model.voxels) {
				s << "   Voxel: ";
				s << std::noshowbase << std::setw(2) << std::setfill('0') << (unsigned)voxel.x << ',';
				s << std::noshowbase << std::setw(2) << std::setfill('0') << (unsigned)voxel.y << ',';
				s << std::noshowbase << std::setw(2) << std::setfill('0') << (unsigned)voxel.z << ',';
				s << "   color=" << std::noshowbase << std::setw(2) << std::setfill('0') << (unsigned)voxel.colorIndex << std::endl;
			}
		}

		s << "Palette: " << (palette == nullptr ? "(DEFAULT)" : "");
		for (uint32_t i = 0; i < palette->size(); i++) {
			if (0 == i % 16) std::cout << std::endl << "   ";
			palette->data()[i].print(std::cout);
			std::cout << "  ";
		}
		std::cout << std::endl;
	}

	std::vector<std::vector<Voxel *>> VoxReader::view2d(const Viewport2d viewport, uint8_t flags, uint32_t modelIndex) const {

		std::vector<std::vector<Voxel *>> view;
		const Model &model = models[modelIndex];

		view.resize(model.sizeX);
		for (auto &xRow : view) {
			xRow.resize(model.sizeZ);
		}

		auto comparator = [&](uint32_t vz, uint32_t vzOther) {

			if (flags & FROM_BEHIND) {
				// -> Greater vZ-values are nearer
				return vz > vzOther;
			}
			else {
				// -> Lower vZ-values are nearer
				return vz < vzOther;
			}
		};

		for (auto voxel : model.voxels) {

			uint8_t vx, vy, vz, vzOther;

			auto invertUp = [&flags](uint32_t v, uint32_t max) {
				return flags & INVERT_UP ? max - v - 1 : v;
			};

			auto fromBehind = [&flags](uint32_t v, uint32_t max) {
				return flags & FROM_BEHIND ? max - v - 1 : v;
			};

			switch (viewport) {
			case XZ:
				vx = fromBehind(voxel.x, model.sizeX);
				vy = invertUp(voxel.z, model.sizeZ);
				vz = voxel.y;
				break;

			case XY:
				vx = fromBehind(voxel.x, model.sizeX);
				vy = invertUp(voxel.y, model.sizeY);
				vz = voxel.z;
				break;

			case YZ:
				vx = fromBehind(voxel.y, model.sizeY);
				vy = invertUp(voxel.z, model.sizeZ);
				vz = voxel.x;
				break;

			default:
				throw Exception("Unkown 2D viewport");
			}

			if (flags & SWAP_AXIS) {
				std::swap(vx, vy);
			}

			if (view[vx][vy] == nullptr) {
				// There's no voxel yet:
				view[vx][vy] = &voxel;
			}
			else {
				switch (viewport) {
				case XZ:
					vzOther = view[vx][vy]->y;
					break;
				case XY:
					vzOther = view[vx][vy]->z;
					break;
				case YZ:
					vzOther = view[vx][vy]->x;
					break;
				}

				if (comparator(vz, vzOther)) {
					// Current voxel is nearer than the already stored one:
					view[vx][vy] = &voxel;
				}
			}
		}

		return std::move(view);
	}

	//////////////////////////////////////////////////////////////////////////////
	// VOXEL
	//////////////////////////////////////////////////////////////////////////////

	Voxel::Voxel(uint8_t x, uint8_t y, uint8_t z, uint8_t colorIndex)
		: x(x), y(y), z(z), colorIndex(colorIndex) {}

	//////////////////////////////////////////////////////////////////////////////
	// MODEL
	//////////////////////////////////////////////////////////////////////////////

	Model::Model(const VoxReader::Chunk &sizeChunk, const VoxReader::Chunk &xyziChunk) {

		// Size:
		sizeX = readInt(&sizeChunk.content[0]);
		sizeY = readInt(&sizeChunk.content[4]);
		sizeZ = readInt(&sizeChunk.content[8]);

		// Voxels:
		uint32_t voxelCount;
		voxelCount = readInt(&xyziChunk.content[0]);

		for (uint32_t i = 1; i <= voxelCount; i++) {
			uint32_t byte = i * 4;
			voxels.push_back(Voxel(
				xyziChunk.content[byte + 0],
				xyziChunk.content[byte + 1],
				xyziChunk.content[byte + 2],
				xyziChunk.content[byte + 3]
			));
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	// RGBA
	//////////////////////////////////////////////////////////////////////////////

	RGBA::RGBA() {
		a = 0;
		r = 0;
		g = 0;
		b = 0;
	}

	RGBA::RGBA(uint32_t color) {
		unpack(color);
	}

	RGBA::RGBA(uint32_t *color) {
		unpack(*color);
	}

	RGBA::RGBA(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
		: a(a), r(r), g(g), b(b) {}

	void RGBA::print(std::ostream & s) const {
		s << std::hex << std::noshowbase << std::setw(8) << std::setfill('0') << pack();
	}

	uint32_t RGBA::pack() const {
		uint32_t ret = (a << 24) | (r << 16) | (g << 8) | b;
		return ret;
	}

	void RGBA::unpack(uint32_t color) {
		a = (color & 0xFF000000) >> 24;
		r = (color & 0x00FF0000) >> 16;
		g = (color & 0x0000FF00) >> 8;
		b = color & 0x000000FF;
	}

	//////////////////////////////////////////////////////////////////////////////
	// Scene Graph
	//////////////////////////////////////////////////////////////////////////////

	template <typename NODE>
	NODE& SceneGraph::addNode(NodeId id) {
		if (id >= nodes.size()) { nodes.resize(id + 1); }
		if (nodes[id]) throw VoxReader::Exception("SceneGraph node duplicate!");
		else nodes[id].reset(new NODE());
		return static_cast<NODE&>(*nodes[id]);
	}

	const SceneGraph::Node* SceneGraph::GetNode(NodeId id) const {
		if (id < 0 || id >= nodes.size()) {
			return nullptr;
		}
		return nodes[id].get();
	}

	void SceneGraph::readTransformNode(const uint8_t* ptr) {
		auto& node = addNode<TransformNode>(readInt(ptr));
		ptr += 4;
		node.attributes = readDictionary(&ptr);
		node.childNodeId = readInt(ptr);
		ptr += 4;
		if (readInt(ptr) != -1) {
			// reserved id (must be -1)
			throw VoxReader::Exception("Expectation not met: reserved id must be -1 (v150 extended spec)");
		}
		ptr += 4;
		node.layerId = readInt(ptr);
		ptr += 4;
		int numOfFrames = readInt(ptr);
		ptr += 4;
		node.frame_attributes.resize(numOfFrames);
		for (int i = 0; i < numOfFrames; ++i)
		{
			node.frame_attributes[i] = readDictionary(&ptr);
		}
	}

	void SceneGraph::readGroupNode(const uint8_t* ptr) {
		auto& node = addNode<GroupNode>(readInt(ptr));
		ptr += 4;
		node.attributes = readDictionary(&ptr);
		int numOfChildren = readInt(ptr);
		ptr += 4;
		node.childNodeIds.resize(numOfChildren);
		for (int i = 0; i < numOfChildren; ++i)
		{
			node.childNodeIds[i] = readInt(ptr);
			ptr += 4;
		}
	}

	void SceneGraph::readShapeNode(const uint8_t* ptr) {
		auto& node = addNode<ShapeNode>(readInt(ptr));
		ptr += 4;
		node.attributes = readDictionary(&ptr);
		int numOfModels = readInt(ptr);
		ptr += 4;
		node.models.resize(numOfModels);
		for (int i = 0; i < numOfModels; ++i)
		{
			node.models[i].modelId = readInt(ptr);
			ptr += 4;
			node.models[i].attributes = readDictionary(&ptr);
		}
	}

} // namespace jim

#endif
