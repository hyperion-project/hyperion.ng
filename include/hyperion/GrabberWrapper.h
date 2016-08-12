#pragma once

#include <QObject>
#include <QTimer>
#include <string>

#include <utils/Logger.h>
#include <utils/Components.h>
#include <hyperion/Hyperion.h>

class ImageProcessor;

class GrabberWrapper : public QObject
{
	Q_OBJECT
public: 
	GrabberWrapper(std::string grabberName, const int priority);
	
	virtual ~GrabberWrapper();
	
	virtual bool start();
	virtual void stop();

public slots:
	void componentStateChanged(const hyperion::Components component, bool enable);
	virtual void action() = 0;

signals:
	void emitImage(int priority, const Image<ColorRgb> & image, const int timeout_ms);

protected:
	std::string _grabberName;
	
	/// Pointer to Hyperion for writing led values
	Hyperion * _hyperion;

	/// The priority of the led colors
	const int _priority;

	/// The timer for generating events with the specified update rate
	QTimer _timer;

	/// The Logger instance
	Logger * _log;
	
	// forwarding enabled
	bool _forward;

	/// The processor for transforming images to led colors
	ImageProcessor * _processor;

};
