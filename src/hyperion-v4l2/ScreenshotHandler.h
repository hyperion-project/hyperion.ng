#pragma once

// Qt includes
#include <QObject>
#include <QRectF>

// hyperionincludes
#include <utils/Image.h>
#include <utils/ColorRgb.h>

/// This class handles callbacks from the V4L2 grabber
class ScreenshotHandler : public QObject
{
	Q_OBJECT

public:
	ScreenshotHandler(const QString & filename, const QRectF & signalDetectionOffset);
	virtual ~ScreenshotHandler();

public slots:
	/// Handle a single image
	/// @param image The image to process
	void receiveImage(const Image<ColorRgb> & image);

private:
	bool findNoSignalSettings(const Image<ColorRgb> & image);

	const QString _filename;
	const QRectF  _signalDetectionOffset;
};
