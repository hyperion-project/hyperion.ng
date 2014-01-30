
// QT includes
#include <QObject>


#include "../hyperion-v4l2/ProtoConnection.h"

class ProtoWrapper : public QObject
{
	Q_OBJECT
public:
	ProtoWrapper(const std::string & protoAddress, const bool skipReply);

public slots:
	void process(const Image<ColorRgb> & image);

private:

	int _priority;
	int _duration_ms;

	ProtoConnection _connection;
};
