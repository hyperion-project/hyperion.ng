
// STL includes
#include <iostream>

// LibPNG includes
#include <png.h>

// Utils includes
#include <utils/RgbImage.h>
#include <utils/jsonschema/JsonFactory.h>

// Raspilight includes
#include <hyperion/Hyperion.h>

// Local includes
#include "FbWriter.h"

bool read_png(std::string file_name, RgbImage*& rgbImage)
{
	png_structp png_ptr;
	png_infop info_ptr;
	FILE *fp;

	if ((fp = fopen(file_name.c_str(), "rb")) == NULL)
	{
		return false;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL)
	{
		fclose(fp);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return false;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return false;
	}

	png_init_io(png_ptr, fp);

	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_SWAP_ALPHA | PNG_TRANSFORM_EXPAND, NULL);

	png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
	png_uint_32 height = png_get_image_height(png_ptr, info_ptr);

	png_uint_32 bitdepth   = png_get_bit_depth(png_ptr, info_ptr);
	png_uint_32 channels   = png_get_channels(png_ptr, info_ptr);
	png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);

	std::cout << "BitDepth" << std::endl;
	std::cout << bitdepth << std::endl;
	std::cout << "Channels" << std::endl;
	std::cout << channels << std::endl;
	std::cout << "ColorType" << std::endl;
	std::cout << color_type << std::endl;

	png_bytepp row_pointers;
	row_pointers = png_get_rows(png_ptr, info_ptr);

	rgbImage = new RgbImage(width, height);

	for (unsigned iRow=0; iRow<height; ++iRow)
	{
		if (color_type == PNG_COLOR_TYPE_RGB)
		{
			RgbColor* rowPtr = reinterpret_cast<RgbColor*>(row_pointers[iRow]);
			for (unsigned iCol=0; iCol<width; ++iCol)
			{
				rgbImage->setPixel(iCol, iRow, rowPtr[iCol]);
			}
		}
		else if (color_type == PNG_COLOR_TYPE_RGBA)
		{
			unsigned* rowPtr = reinterpret_cast<unsigned*>(row_pointers[iRow]);
			for (unsigned iCol=0; iCol<width; ++iCol)
			{
				const unsigned argbValue = rowPtr[iCol];
				rgbImage->setPixel(iCol, iRow, {uint8_t((argbValue >> 16) & 0xFF), uint8_t((argbValue >> 8) & 0xFF), uint8_t((argbValue) & 0xFF)});
			}
		}
		else
		{
			// Unknown/Unimplemented color-format
			return false;
		}
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);

	return true;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		// Missing required argument
		std::cout << "Missing PNG-argumet. Usage: 'ViewPng [png-file]" << std::endl;
		return 0;
	}

	const std::string pngFilename = argv[1];

	RgbImage* image = nullptr;
	if (!read_png(pngFilename, image) || image == nullptr)
	{
		std::cout << "Failed to load image" << std::endl;
		return -1;
	}

	const char* homeDir = getenv("RASPILIGHT_HOME");
	if (!homeDir)
	{
		homeDir = "/home/pi";
	}

	std::cout << "RASPILIGHT HOME DIR: " << homeDir << std::endl;

	const std::string schemaFile = std::string(homeDir) + "/hyperion.schema.json";
	const std::string configFile = std::string(homeDir) + "/hyperion.config.json";

	Json::Value raspiConfig;
	if (JsonFactory::load(schemaFile, configFile, raspiConfig) < 0)
	{
		std::cerr << "UNABLE TO LOAD CONFIGURATION" << std::endl;
		return -1;
	}
	std::cout << "Loaded configuration: " << raspiConfig << std::endl;

	FbWriter fbWriter;

	Hyperion raspiLight(raspiConfig);
	raspiLight.setInputSize(image->width(), image->height());

	fbWriter.writeImage(*image);
	raspiLight(*image);

	sleep(5);

	delete image;
}
