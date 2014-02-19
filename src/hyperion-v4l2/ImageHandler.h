// Qt includes
#include <QObject>

// hyperion includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>

// blackborder includes
#include <blackborder/BlackBorderProcessor.h>

// hyperion v4l2 includes
#include "ProtoConnection.h"

/// This class handles callbacks from the V4L2 grabber
class ImageHandler : public QObject
{
	Q_OBJECT

public:
	ImageHandler(const std::string & address, int priority, double signalThreshold, bool skipProtoReply);
	virtual ~ImageHandler();

public slots:
	/// Handle a single image
	/// @param image The image to process
	void receiveImage(const Image<ColorRgb> & image);

private:
	/// Priority for calls to Hyperion
	const int _priority;

	/// Hyperion proto connection object
	ProtoConnection _connection;

	/// Threshold used for signal detection
	double _signalThreshold;

	/// Blackborder detector which is used as a signal detector (unknown border = no signal)
	hyperion::BlackBorderProcessor _signalProcessor;
};
