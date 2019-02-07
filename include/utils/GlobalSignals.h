#pragma once

// util
#include <utils/Image.h>
#include <utils/ColorRgb.h>

// qt
#include <QObject>

///
/// Singleton instance for simple signal sharing across threads, should be never used with Qt:DirectConnection!
///
class GlobalSignals : public QObject
{
	Q_OBJECT
public:
    static GlobalSignals* getInstance()
    {
        static GlobalSignals instance;
        return & instance;
    }
private:
    GlobalSignals() {}

public:
    GlobalSignals(GlobalSignals const&)   = delete;
    void operator=(GlobalSignals const&)  = delete;

signals:
	///
	/// @brief PIPE SystemCapture images from GrabberWrapper to Hyperion class
	/// @param image  The prepared image
	///
	void setSystemImage(const Image<ColorRgb>&  image);

	///
	/// @brief PIPE v4lCapture images from v4lCapture over HyperionDaemon to Hyperion class
	/// @param image  The prepared image
	///
	void setV4lImage(const Image<ColorRgb> & image);
};
