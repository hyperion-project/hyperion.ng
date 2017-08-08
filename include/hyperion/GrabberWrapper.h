#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <QStringList>

#include <utils/Logger.h>
#include <utils/Components.h>
#include <utils/GrabbingMode.h>
#include <hyperion/Hyperion.h>

class ImageProcessor;
class Grabber;

class GrabberWrapper : public QObject
{
	Q_OBJECT
public: 
	GrabberWrapper(QString grabberName, const unsigned updateRate_Hz, const int priority, hyperion::Components grabberComponentId=hyperion::COMP_GRABBER);
	
	virtual ~GrabberWrapper();
	
	///
	/// Starts the grabber wich produces led values with the specified update rate
	///
	virtual bool start();

	///
	/// Stop grabber
	///
	virtual void stop();

	static QStringList availableGrabbers();

public slots:
	void componentStateChanged(const hyperion::Components component, bool enable);
	
	///
	/// virtual method, should perform single frame grab and computes the led-colors
	///
	virtual void action() = 0;

	///
	/// Set the grabbing mode
	/// @param[in] mode The new grabbing mode
	///
	void setGrabbingMode(const GrabbingMode mode);

	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	virtual void setVideoMode(const VideoMode videoMode);

	virtual void setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom);

signals:
	void emitImage(int priority, const Image<ColorRgb> & image, const int timeout_ms);

protected:

	void setColors(const std::vector<ColorRgb> &ledColors, const int timeout_ms);
	QString _grabberName;
	
	/// Pointer to Hyperion for writing led values
	Hyperion * _hyperion;

	/// The priority of the led colors
	const int _priority;

	/// The timer for generating events with the specified update rate
	QTimer _timer;

	/// The update rate [Hz]
	const int _updateInterval_ms;

	/// The timeout of the led colors [ms]
	const int _timeout_ms;

	/// The Logger instance
	Logger * _log;
	
	// forwarding enabled
	bool _forward;

	/// The processor for transforming images to led colors
	ImageProcessor * _processor;

	hyperion::Components _grabberComponentId;
	
	Grabber *_ggrabber;

	/// The image used for grabbing frames
	Image<ColorRgb> _image;
};
