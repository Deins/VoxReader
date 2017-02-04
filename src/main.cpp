#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "VoxReader.h"

int main() {
	using namespace jim;

	VoxReader vox;

	try {
		vox.load(std::ifstream("3x3x3_palette.vox", std::ios::binary));
	} catch(const std::exception &e) {
		std::cout << e.what() << std::endl;
	}

	vox.print(std::cout);

	std::cin.ignore(10, '\n');
}
