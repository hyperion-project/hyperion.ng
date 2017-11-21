// Qt includes
#include <QObject>

// hyperion includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/VideoMode.h>

// hyperion proto includes
#include "protoserver/ProtoConnection.h"

/// This class handles callbacks from the V4L2 and X11 grabber
class ProtoConnectionWrapper : public QObject
{
	Q_OBJECT

public:
	ProtoConnectionWrapper(const QString &address, int priority, int duration_ms, bool skipProtoReply);
	virtual ~ProtoConnectionWrapper();

signals:
	///
	/// Forwarding new videoMode
	///
	void setVideoMode(const VideoMode videoMode);

public slots:
	/// Handle a single image
	/// @param image The image to process
	void receiveImage(const Image<ColorRgb> & image);

private:
	/// Priority for calls to Hyperion
	const int _priority;

	/// Duration for color calls to Hyperion
	const int _duration_ms;

	/// Hyperion proto connection object
	ProtoConnection _connection;
};
