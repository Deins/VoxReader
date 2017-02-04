#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "VoxReader.h"

int main() {
	using namespace jim;

	VoxReader vox;

	try {
		vox.load(std::ifstream("chr_knight.vox", std::ios::binary));
	} catch(const std::exception &e) {
		std::cout << e.what() << std::endl;
	}

	vox.print(std::cout);
	std::cout << std::endl << std::endl;

	std::cout << "XZ-View:" << std::endl;
	auto view = vox.view2d(VoxReader::XZ, VoxReader::SWAP_AXIS | VoxReader::INVERT_UP | VoxReader::FROM_BEHIND);
	for(const auto &row : view) {
		for(const auto *voxel : row) {
			if(voxel != nullptr) {
				std::cout << 'X';
			} else {
				std::cout << ' ';
			}
		}
		std::cout << std::endl;
	}

	std::cin.ignore(10, '\n');
}
