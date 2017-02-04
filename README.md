# VoxReader
Vox-file parser

Loads a vox-file from disk into memory, representing voxel data as objects.
Multiple models per file are supported.

Currently, the loader reads only the voxels themselves and the color palette,
but ignores materials if present.

The loader is intended to be used in my further projects, like my voxel-fractal generator.
Therefore, further development is planned.

Vox-file format spec: https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
