#pragma once

#include <HyperionConfig.h> // Required to determine the cmake options
#include <hyperion/GrabberWrapper.h>

#if defined(ENABLE_MF)
	#include <grabber/MFGrabber.h>
#elif defined(ENABLE_V4L2)
	#include <grabber/V4L2Grabber.h>
#endif

#if defined(ENABLE_CEC)
	#include <cec/CECEvent.h>
#endif

class VideoWrapper : public GrabberWrapper
{
	Q_OBJECT

public:
	VideoWrapper();
	~VideoWrapper() override;

public slots:
	bool start() override;
	void stop() override;

#if defined(ENABLE_CEC)
	void handleCecEvent(CECEvent event);
#endif

	void handleSettingsUpdate(settings::type type, const QJsonDocument& config) override;

private slots:
	void newFrame(const Image<ColorRgb> & image);
	void readError(const char* err);

	void action() override;

private:
	/// The Media Foundation or V4L2 grabber
#if defined(ENABLE_MF)
	MFGrabber _grabber;
#elif defined(ENABLE_V4L2)
	V4L2Grabber _grabber;
#endif
};
