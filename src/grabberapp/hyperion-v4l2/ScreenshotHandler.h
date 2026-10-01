#pragma once

// Qt includes
#include <QObject>
#include <QRectF>

// hyperionincludes
#include <utils/Image.h>
#include <utils/ColorRgb.h>

/// Analyses a single frame captured for --screenshot and prints "no signal area"
/// suggestion diagnostics to stdout. Saving the actual screenshot file and quitting
/// the application is handled generically by the shared GrabberRunner/V4L2Wrapper code.
class ScreenshotHandler : public QObject
{
	Q_OBJECT

public:
	explicit ScreenshotHandler(const QRectF & signalDetectionOffset);

public slots:
	/// Handle a single image
	/// @param image The image to process
	void receiveImage(const Image<ColorRgb> & image);

private:
	bool findNoSignalSettings(const Image<ColorRgb> & image);

	const QRectF  _signalDetectionOffset;
};
