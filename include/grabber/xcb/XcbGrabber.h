#pragma once

#include <QAbstractNativeEventFilter>
// QT includes
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include <utils/ColorRgb.h>
#include <hyperion/Grabber.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

class Logger;

class XcbGrabber : public Grabber, public QAbstractNativeEventFilter
{
	Q_OBJECT

public:
	XcbGrabber(int cropLeft=0, int cropRight=0, int cropTop=0, int cropBottom=0);

	~XcbGrabber() override;

	bool open();
	bool setupDisplay();

	int grabFrame(Image<ColorRgb> & image, bool forceUpdate = false);
	int updateScreenDimensions(bool force = false);
	void setVideoMode(VideoMode mode) override;
	bool setWidthHeight(int width, int height) override { return true; }
	bool setPixelDecimation(int pixelDecimation) override;
	void setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom) override;

	///
	/// @brief Discover XCB screens available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params);

private:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	bool nativeEventFilter(const QByteArray & eventType, void * message, qintptr * result) override;
#else
	bool nativeEventFilter(const QByteArray & eventType, void * message, long int * result) override;
#endif
	void freeResources();
	void setupResources();
	void setupRender();
	void setupRandr();
	void setupShm();
	xcb_screen_t * getScreen(const xcb_setup_t *setup, int screen_num) const;
	xcb_render_pictformat_t findFormatForVisual(xcb_visualid_t visual) const;

	xcb_connection_t * _connection;
	xcb_screen_t * _screen;
	xcb_pixmap_t _pixmap;
	xcb_render_pictformat_t _srcFormat;
	xcb_render_pictformat_t _dstFormat;
	xcb_render_picture_t _srcPicture;
	xcb_render_picture_t _dstPicture;
	xcb_render_transform_t _transform;
	xcb_shm_seg_t  _shminfo;

	int _screen_num;
	unsigned _screenWidth;
	unsigned _screenHeight;
	unsigned _src_x;
	unsigned _src_y;

	bool _XcbRenderAvailable;
	bool _XcbRandRAvailable;
	bool _XcbShmAvailable;
	bool _XcbShmPixmapAvailable;
	bool _isWayland;

	Logger * _logger;

	uint8_t * _shmData;

	int _XcbRandREventBase;
};
