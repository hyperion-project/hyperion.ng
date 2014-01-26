// blackborder includes
#include <blackborder/BlackBorderProcessor.h>

// hyperion-v4l includes
#include "ProtoConnection.h"

/// This class handles callbacks from the V4L2 grabber
class ImageHandler
{
public:
	ImageHandler(const std::string & address, int priority, double signalThreshold, bool skipProtoReply);

	/// Handle a single image
	/// @param image The image to process
	void receiveImage(const Image<ColorRgb> & image);

	/// static function used to direct callbacks to a ImageHandler object
	/// @param arg This should be an ImageHandler instance
	/// @param image The image to process
	static void imageCallback(void * arg, const Image<ColorRgb> & image);

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
