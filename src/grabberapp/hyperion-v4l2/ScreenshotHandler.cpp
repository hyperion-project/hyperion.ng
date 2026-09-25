#include <cmath>

// Qt includes
#include <QImage>
#include <QCoreApplication>
#include <QVector>
#include <algorithm>

// hyperion-v4l2 includes
#include "ScreenshotHandler.h"

ScreenshotHandler::ScreenshotHandler(const QString & filename, const QRectF & signalDetectionOffset)
	: _filename(filename)
	, _signalDetectionOffset(signalDetectionOffset)
{
}

ScreenshotHandler::~ScreenshotHandler()
{
}

void ScreenshotHandler::receiveImage(const Image<ColorRgb> & image)
{
	findNoSignalSettings(image);
	// store as PNG
	QImage pngImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
	pngImage.save(_filename);

	// Quit the application after the first image
	QCoreApplication::quit();
}

bool ScreenshotHandler::findNoSignalSettings(const Image<ColorRgb> & image)
{
	double x_frac_min = _signalDetectionOffset.x();
	double y_frac_min = _signalDetectionOffset.y();
	double x_frac_max = _signalDetectionOffset.width();
	double y_frac_max = _signalDetectionOffset.height();

	unsigned xOffset  = image.width()  * x_frac_min;
	unsigned yOffset  = image.height() * y_frac_min;
	unsigned xMax     = image.width()  * x_frac_max;
	unsigned yMax     = image.height() * y_frac_max;

	ColorRgb noSignalThresholdColor = {0,0,0};

	unsigned yMid = (yMax+yOffset) / 2;
	ColorRgb redThresoldColor   = {255,75,75};
	ColorRgb greenThresoldColor = {75,255,75};
	ColorRgb blueThresoldColor  = {75,75,255};

	QVector<unsigned> redOffsets;
	QVector<unsigned> redCounts;
	QVector<unsigned> greenOffsets;
	QVector<unsigned> greenCounts;
	QVector<unsigned> blueOffsets;
	QVector<unsigned> blueCounts;

	unsigned currentColor = 255;
	for (unsigned x = xOffset; x < xMax; ++x)
	{
		ColorRgb rgb = image(x, yMid);
		if (rgb <= redThresoldColor)
		{
			if ( currentColor != 0)
			{
				redOffsets.append(x);
				redCounts.append(1);
			}
			else
			{
				redCounts[redCounts.size()-1]++;
			}
			currentColor = 0;
		}
		if (rgb <= greenThresoldColor){
			if ( currentColor != 1)
			{
				greenOffsets.append(x);
				greenCounts.append(1);
			}
			else
			{
				greenCounts[greenCounts.size()-1]++;
			}
			currentColor = 1;
		}
		if (rgb <= blueThresoldColor)
		{
			if ( currentColor != 2)
			{
				blueOffsets.append(x);
				blueCounts.append(1);
			}
			else
			{
				blueCounts[blueCounts.size()-1]++;
			}
			currentColor = 2;
		}
	}

	auto itR = std::max_element(std::begin(redCounts), std::end(redCounts));
	auto itG = std::max_element(std::begin(greenCounts), std::end(greenCounts));
	auto itB = std::max_element(std::begin(blueCounts), std::end(blueCounts));

	double xOffsetSuggested = xOffset;
	double yOffsetSuggested = yOffset;
	double xMaxSuggested    = xMax;
	double yMaxSuggested    = yMax;
	bool   noSignalBlack    = false;

	noSignalThresholdColor = {0,0,0};
	if (*itR >= *itG && *itR >=  *itB && *itR > 1)
	{
		xOffsetSuggested       = redOffsets[redCounts.indexOf(*itR)];
		xMaxSuggested          = xOffsetSuggested + *itR;
		noSignalThresholdColor = redThresoldColor;
	}
	else if (*itG >= *itR && *itG >=  *itB && *itG > 1 )
	{
		xOffsetSuggested       = greenOffsets[greenCounts.indexOf(*itG)];
		xMaxSuggested          = xOffsetSuggested + *itG;
		noSignalThresholdColor = greenThresoldColor;
	}
	else if ( *itB > 1 )
	{
		xOffsetSuggested       = blueOffsets[blueCounts.indexOf(*itB)];
		xMaxSuggested          = xOffsetSuggested + *itB;
		noSignalThresholdColor = blueThresoldColor;
	}
	else
	{
		noSignalThresholdColor = {75,75,75};
		noSignalBlack = true;
	}

	// serach vertical max
	if (!noSignalBlack)
	{
		unsigned xMid = (xMaxSuggested + xOffsetSuggested) / 2;
		for (unsigned y = yMid; y >= yOffset && (fabs(yOffsetSuggested - y) > std::numeric_limits<double>::epsilon()); --y)
		{
			ColorRgb rgb = image(xMid, y);
			if (rgb <= noSignalThresholdColor)
			{
				yOffsetSuggested = y;
			}
		}

		for (unsigned y = yMid; y <= yMax && (fabs(yMaxSuggested - y) > std::numeric_limits<double>::epsilon()); ++y)
		{
			ColorRgb rgb = image(xMid, y);
			if (rgb <= noSignalThresholdColor)
			{
				yMaxSuggested = y;
			}
		}
	}

	// optimize thresold color
	noSignalThresholdColor = {0,0,0};
	for (unsigned x = xOffsetSuggested; x < xMaxSuggested; ++x)
	{
		for (unsigned y = yOffsetSuggested; y < yMaxSuggested; ++y)
		{
			ColorRgb rgb = image(x, y);
			if (rgb >= noSignalThresholdColor)
			{
				noSignalThresholdColor = rgb;
			}
		}
	}

	// calculate fractional values
	xOffsetSuggested = (int)(((float)xOffsetSuggested/image.width())*100+0.5)/100.0;
	xMaxSuggested    = (int)(((float)xMaxSuggested/image.width())*100)/100.0;
	yOffsetSuggested = (int)(((float)yOffsetSuggested/image.height())*100+0.5)/100.0;
	yMaxSuggested    = (int)(((float)yMaxSuggested/image.height())*100)/100.0;
	double thresholdRed   = (int)(((float)noSignalThresholdColor.red/255.0f)*100+0.5)/100.0;
	double thresholdGreen = (int)(((float)noSignalThresholdColor.green/255.0f)*100+0.5)/100.0;
	double thresholdBlue  = (int)(((float)noSignalThresholdColor.blue/255.0f)*100+0.5)/100.0;
	thresholdRed   = (thresholdRed<0.1f)  ?0.1f : thresholdRed;
	thresholdGreen = (thresholdGreen<0.1f)?0.1f : thresholdGreen;
	thresholdBlue  = (thresholdBlue<0.1f) ?0.1f : thresholdBlue;

	std::cout << std::endl << "Signal detection informations"
	          << std::endl << "============================="
	          << std::endl << "dimension after decimation: " << image.width() << " x " << image.height()
	          << std::endl << "signal detection area  : " << xOffset << "," << yOffset << " x "  << xMax << "," << yMax  << std::endl << std::endl;

	// check if values make sense
	if (thresholdRed < 0.5 && thresholdGreen < 0.5 && thresholdBlue < 0.5 && thresholdRed > 0.15 && thresholdGreen > 0.15 && thresholdBlue > 0.15)
	{
		std::cout << "WARNING \"no signal image\" is to dark, signal detection is not relaiable." << std::endl;
	}

	if (thresholdRed > 0.5 && thresholdGreen > 0.5 && thresholdBlue > 0.5)
	{
		std::cout << "WARNING \"no signal image\" is to bright, signal detection is not relaiable." << std::endl;
	}

	if (thresholdRed > thresholdGreen && thresholdRed > thresholdBlue && ((thresholdRed-thresholdGreen) <= 0.5 || (thresholdRed-thresholdBlue) <= 0.5))
	{
		std::cout << "WARNING difference between threshold color and the other color components is to small, signal detection might have problems." << std::endl;
	}

	if (thresholdGreen > thresholdRed && thresholdGreen > thresholdBlue && ((thresholdGreen-thresholdRed) <= 0.5 || (thresholdGreen-thresholdBlue) <= 0.5))
	{
		std::cout << "WARNING difference between threshold color and the other color components is to small, signal detection might have problems." << std::endl;
	}

	if (thresholdBlue > thresholdGreen && thresholdBlue > thresholdRed && ((thresholdBlue-thresholdGreen) <= 0.5 || (thresholdBlue-thresholdRed) <= 0.5))
	{
		std::cout << "WARNING difference between threshold color and the other color components is to small, signal detection might have problems." << std::endl;
	}

	if (noSignalBlack)
	{
		std::cout << "WARNING no red, green or blue \"no signal area\" detected, signal detection might have problems." << std::endl;
	}

	if (xOffsetSuggested >= xMaxSuggested || (xMaxSuggested - xOffsetSuggested) < 0.029 )
	{
		std::cout << "WARNING horizontal values of signal detection are invalid or detection area is to small, signal detection is not relaiable." << std::endl;
	}

	if (yOffsetSuggested >= yMaxSuggested || (yMaxSuggested - yOffsetSuggested) < 0.029 )
	{
		std::cout << "WARNING horizontal values of signal detection are invalid or detection area is to small, signal detection is not relaiable." << std::endl;
	}

	std::cout << std::endl
	          << "suggested config values for signal detection:" << std::endl
	          << "\t\"redSignalThreshold\"   : "               << thresholdRed     << "," << std::endl
	          << "\t\"greenSignalThreshold\" : "               << thresholdGreen   << "," << std::endl
	          << "\t\"blueSignalThreshold\"  : "               << thresholdBlue    << "," << std::endl
	          << "\t\"signalDetectionHorizontalOffsetMin\" : " << xOffsetSuggested << "," << std::endl
	          << "\t\"signalDetectionVerticalOffsetMin\"   : " << yOffsetSuggested << "," << std::endl
	          << "\t\"signalDetectionHorizontalOffsetMax\" : " << xMaxSuggested    << "," << std::endl
	          << "\t\"signalDetectionVerticalOffsetMax\"   : " << yMaxSuggested    << std::endl;

	return true;
}



