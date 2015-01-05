// Qt includes
#include <QObject>

// hyperion includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>

// hyperion proto includes
#include "protoserver/ProtoConnection.h"

/// This class handles callbacks from the V4L2 grabber
class ProtoConnectionWrapper : public QObject
{
    Q_OBJECT

public:
    ProtoConnectionWrapper(const std::string & address, int priority, int duration_ms, bool skipProtoReply);
    virtual ~ProtoConnectionWrapper();

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
