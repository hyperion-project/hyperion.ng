// Qt includes
#include <QObject>

// hyperionincludes
#include <utils/Image.h>
#include <utils/ColorRgb.h>

/// This class handles callbacks from the V4L2 grabber
class ScreenshotHandler : public QObject
{
	Q_OBJECT

public:
	ScreenshotHandler(const std::string & filename);
	virtual ~ScreenshotHandler();

public slots:
	/// Handle a single image
	/// @param image The image to process
	void receiveImage(const Image<ColorRgb> & image);

private:
	const std::string _filename;
};
