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

/// @brief Analyses a captured image to suggest optimal signal-detection settings
///        for the V4L2 grabber's "no signal" detection feature.
///
/// The function examines the image (typically a frame captured while no video
/// signal is present — e.g. a solid-coloured screen-saver or blank input) and
/// derives the best rectangular detection area and colour threshold values to
/// use in the Hyperion configuration.  Results are printed to @c std::cout.
///
/// @par Algorithm overview
/// 1. **Horizontal colour-run detection** — a horizontal scan is performed along
///    the vertical midpoint of the configured detection area.  Consecutive pixels
///    that fall below the red, green, or blue threshold colours are grouped into
///    runs.  The channel with the longest single run is selected as the dominant
///    colour, establishing the horizontal extent (@c xOffsetSuggested,
///    @c xMaxSuggested) of the no-signal region.
///
/// 2. **Vertical extent search** — if a dominant colour was found, the function
///    walks upward and downward from the horizontal midpoint along the dominant
///    column to find how far the uniform region extends vertically
///    (@c yOffsetSuggested, @c yMaxSuggested).
///
/// 3. **Threshold colour optimisation** — the maximum (brightest) pixel within
///    the suggested rectangle is found and used as the refined threshold colour.
///    This ensures the threshold is just high enough to encompass all pixels in
///    the no-signal area.
///
/// 4. **Sanity checks** — several warnings are printed when the derived values
///    are likely to produce unreliable signal detection:
///    - Threshold too dark (< 0.15 normalised) or too bright (> 0.50).
///    - Insufficient contrast between the dominant channel and the others.
///    - No dominant coloured region detected (falls back to a grey threshold).
///    - Suggested detection rectangle is empty or too narrow (< ~3 % of frame).
///
/// 5. **Output** — the suggested config keys and values are printed in a format
///    ready to copy into a Hyperion JSON configuration file.
///
/// @param image The captured image to analyse.  Coordinates are in the
///              decimated output space (after @c ImageResampler::processImage).
/// @return Always returns @c true (reserved for future error reporting).
bool ScreenshotHandler::findNoSignalSettings(const Image<ColorRgb> & image)
{
	// Convert the fractional signal-detection offsets to absolute pixel coordinates.
	double x_frac_min = _signalDetectionOffset.x();
	double y_frac_min = _signalDetectionOffset.y();
	double x_frac_max = _signalDetectionOffset.width();
	double y_frac_max = _signalDetectionOffset.height();

	unsigned xOffset  = image.width()  * x_frac_min;
	unsigned yOffset  = image.height() * y_frac_min;
	unsigned xMax     = image.width()  * x_frac_max;
	unsigned yMax     = image.height() * y_frac_max;

	ColorRgb noSignalThresholdColor = {0,0,0};

	// Scan along the vertical midpoint of the detection area.
	unsigned yMid = (yMax+yOffset) / 2;
	// Per-channel threshold colours: a pixel is considered "mostly red/green/blue"
	// when all three of its components are at or below the respective threshold.
	ColorRgb redThresoldColor   = {255,75,75};
	ColorRgb greenThresoldColor = {75,255,75};
	ColorRgb blueThresoldColor  = {75,75,255};

	// Run-length encoding accumulators for each colour channel.
	// "Offsets" stores the x-coordinate where each run starts;
	// "Counts" stores how many consecutive pixels are in that run.
	QVector<unsigned> redOffsets;
	QVector<unsigned> redCounts;
	QVector<unsigned> greenOffsets;
	QVector<unsigned> greenCounts;
	QVector<unsigned> blueOffsets;
	QVector<unsigned> blueCounts;

	// Step 1: Horizontal scan — accumulate colour runs along yMid.
	// currentColor tracks which channel's run is currently open (255 = none).
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



