#include <cstring>
#include "cxxopts.hpp"
#include <iostream>
#include "gli/type.hpp"
//#include "gli/texture2d.hpp"
#include "gli/convert.hpp"
#include "gli/copy.hpp"
#include "gli/generate_mipmaps.hpp"
#include "gli/save.hpp"
#include <FreeImage.h>


///////////////////////////////////////////////////////

int main(int argc, char* argv[]){
	int numMipmaps = 0;
	std::string inputFile = "";
	std::string outputFile = "";

	try{
		cxxopts::Options options("textureConverter", "Convert image files to textures");

		options.add_options()
				("i,input", "input file", cxxopts::value<std::string>(inputFile))
				("o,output", "output file", cxxopts::value<std::string>(outputFile))
				("m,mipmaps", "number of mipmaps to generate, 0 to disable", cxxopts::value<int>(numMipmaps));

		options.parse(argc, argv);

		if(!options.count("input") || !options.count("output")){
			std::cout << options.help() << std::endl;
			exit(0);
		}

	} catch (const cxxopts::OptionException& e) {
		std::cout << "error parsing options: " << e.what() << std::endl;
		exit(1);
	}

	if(std::strstr(inputFile.c_str(), ".png") == nullptr){
		std::cerr << "in file must be png" << std::endl;
		return 1;
	}

	if(std::strstr(outputFile.c_str(), ".dds") == nullptr && std::strstr(outputFile.c_str(), ".ktx") == nullptr){
		std::cerr << "out file must be .dds or .ktx" << std::endl;
		return 1;
	}


	// load png
	FreeImage_Initialise(false);


	FIBITMAP* bmp = FreeImage_Load(FIF_PNG, inputFile.c_str(), 0);
	if(!bmp)
		return 0;

	glm::uint BPP = FreeImage_GetBPP(bmp);
	glm::uint Width = FreeImage_GetWidth(bmp);
	glm::uint Height = FreeImage_GetHeight(bmp);

	auto format = gli::FORMAT_RGB8_UNORM_PACK8;
	if(BPP == 32)
		format = gli::FORMAT_RGBA8_UNORM_PACK8;
	if(BPP == 8)
		format = gli::FORMAT_R8_UNORM_PACK8;

	gli::texture2d tex(format, gli::texture2d::extent_type(Width, Height), 1);
	memcpy(tex.data(), FreeImage_GetBits(bmp), tex.size());


	// autogenerate mipmaps
	if(numMipmaps>0){
		//create a new texture with mip map storage
		gli::texture2d newTex(tex.format(), tex.extent(), numMipmaps);

		// copy original size into data
		memcpy(newTex.data(), tex.data(), tex.size());

		// generate smooth resizes with freeimage
		for(gli::texture::size_type level=1; level<newTex.levels(); level++){
			//std::cout << newTex.extent(level).y << std::endl;
			auto size = newTex.extent(level);
			auto data = newTex.data(0, 0, level);
			auto scaled = FreeImage_Rescale(bmp, size.x, size.y, FREE_IMAGE_FILTER::FILTER_LANCZOS3);
			memcpy(data, FreeImage_GetBits(scaled), newTex.size(level));
			FreeImage_Unload(scaled);
		}
		gli::save(newTex, outputFile);

	}else{
		gli::save(tex, outputFile);
	}

	FreeImage_Unload(bmp);

	FreeImage_DeInitialise();

	return 0;
}
