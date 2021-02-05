#pragma once

#include <QObject>
#include <cstdint>

#include <utils/ColorRgb.h>
#include <utils/Image.h>
#include <utils/VideoMode.h>
#include <grabber/VideoStandard.h>
#include <utils/ImageResampler.h>
#include <utils/Logger.h>
#include <utils/Components.h>

#include <QMultiMap>

///
/// @brief The Grabber class is responsible to apply image resizes (with or without ImageResampler)

class Grabber : public QObject
{
	Q_OBJECT

public:
	Grabber(const QString& grabberName = "", int width=0, int height=0, int cropLeft=0, int cropRight=0, int cropTop=0, int cropBottom=0);

	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	virtual void setVideoMode(VideoMode mode);

	///
	/// Apply new flip mode (vertical/horizontal/both)
	/// @param[in] mode The new flip mode
	///
	virtual void setFlipMode(FlipMode mode);

	///
	/// @brief Apply new crop values, on errors reject the values
	///
	virtual void setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom);

	///
	/// @brief Apply new video input (used from v4l)
	/// @param input device input
	///
	virtual bool setInput(int input);

	///
	/// @brief Apply new width/height values, on errors (collide with cropping) reject the values
	/// @return True on success else false
	///
	virtual bool setWidthHeight(int width, int height);

	///
	/// @brief Apply new framerate (used from v4l)
	/// @param fps framesPerSecond
	///
	virtual bool setFramerate(int fps);

	///
	/// @brief Apply new pixelDecimation (used from x11, xcb and qt)
	///
	virtual void setPixelDecimation(int pixelDecimation) {}

	///
	/// @brief Apply new signalThreshold (used from v4l)
	///
	virtual void setSignalThreshold(
					double redSignalThreshold,
					double greenSignalThreshold,
					double blueSignalThreshold,
					int noSignalCounterThreshold = 50) {}
	///
	/// @brief Apply new SignalDetectionOffset  (used from v4l)
	///
	virtual void setSignalDetectionOffset(
					double verticalMin,
					double horizontalMin,
					double verticalMax,
					double horizontalMax) {}

	///
	/// @brief Apply SignalDetectionEnable (used from v4l)
	///
	virtual void setSignalDetectionEnable(bool enable) {}

	///
	/// @brief Apply CecDetectionEnable (used from v4l)
	///
	virtual void setCecDetectionEnable(bool enable) {}

	///
	/// @brief Apply device and videoStandard (used from v4l)
	///
	virtual void setDeviceVideoStandard(QString device, VideoStandard videoStandard) {}

	///
	/// @brief Apply device (used from MediaFoundation)
	///
	virtual bool setDevice(QString device) { return false; }

	///
	/// @brief Apply display index (used from qt)
	///
	virtual void setDisplayIndex(int index) {}

	///
	/// @brief Apply path for device (used from framebuffer)
	///
	virtual void setDevicePath(const QString& path) {}

	///
	/// @brief get current resulting height of image (after crop)
	///
	virtual int getImageWidth() { return _width; }

	///
	/// @brief get current resulting width of image (after crop)
	///
	virtual int getImageHeight() { return _height; }

	///
	/// @brief Prevent the real capture implementation from capturing if disabled
	///
	void setEnabled(bool enable);

	///
	/// @brief Get a list of all available devices
	/// @return List of all available devices on success else empty List
	///
	virtual QStringList getDevices() const { return QStringList(); }

	///
	/// @brief Get the device name by path
	/// @param devicePath The device path
	/// @return The name of the device on success else empty String
	///
	virtual QString getDeviceName(const QString& /*devicePath*/) const { return QString(); }

	///
	/// @brief Get a name/index pair of supported device inputs
	/// @param devicePath The device path
	/// @return multi pair of name/index on success else empty pair
	///
	virtual QMultiMap<QString, int> getDeviceInputs(const QString& /*devicePath*/) const { return QMultiMap<QString, int>(); }

	///
	/// @brief Get a list of all available device encoding formats depends on device input
	/// @param devicePath The device path
	/// @param inputIndex The device input index
	/// @return List of device encoding formats on success else empty List
	///
	virtual QStringList getAvailableEncodingFormats(const QString& /*devicePath*/, const int& /*deviceInput*/) const { return QStringList(); }

	///
	/// @brief Get a map of available device resolutions (width/heigth) depends on device input and encoding format
	/// @param devicePath The device path
	/// @param inputIndex The device input index
	/// @param encFormat The device encoding format
	/// @return Map of resolutions (width/heigth) on success else empty List
	///
	virtual QMultiMap<int, int> getAvailableDeviceResolutions(const QString& /*devicePath*/, const int& /*deviceInput*/, const PixelFormat& /*encFormat*/) const { return QMultiMap<int, int>(); }

	///
	/// @brief Get a list of available device framerates depends on device input, encoding format and resolution
	/// @param devicePath The device path
	/// @param inputIndex The device input index
	/// @param encFormat The device encoding format
	/// @param width The device width
	/// @param heigth The device heigth
	/// @return List of framerates on success else empty List
	///
	virtual QStringList getAvailableDeviceFramerates(const QString& /*devicePath*/, const int& /*deviceInput*/, const PixelFormat& /*encFormat*/, const unsigned /*width*/, const unsigned /*height*/) const { return QStringList(); }

protected:
	ImageResampler _imageResampler;

	bool _useImageResampler;

	/// The selected VideoMode
	VideoMode _videoMode;

	/// The used Flip Mode
	FlipMode _flipMode;

	/// With of the captured snapshot [pixels]
	int _width;

	/// Height of the captured snapshot [pixels]
	int _height;

	/// frame per second
	int _fps;

	/// device input
	int _input;

	/// number of pixels to crop after capturing
	int _cropLeft, _cropRight, _cropTop, _cropBottom;

	bool _enabled;

	/// logger instance
	Logger * _log;

};
