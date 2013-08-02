
#pragma once

// Utils includes
#include <utils/RgbImage.h>

// Forward class declaration
class pngwriter;

/**
 * @brief The HyperionPng class implements the same interface
 */
class HyperionPng
{
public:
	HyperionPng();

	~HyperionPng();

	void setInputSize(const unsigned width, const unsigned height);

	RgbImage& image();

	void commit();

	void operator() (const RgbImage& inputImage);


private:
	RgbImage* mBuffer;

	unsigned mFrameCnt;
	unsigned mWriteFrequency;

	pngwriter *mWriter;
	unsigned long mFileIndex;

	HyperionPng(const HyperionPng&)
	{
		// empty
	}

	HyperionPng& operator=(const HyperionPng&)
	{
		return *this;
	}
};
