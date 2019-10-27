#pragma once

// util
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/Components.h>
#include <utils/VideoMode.h>
#include <api/API.h>

// qt
#include <QObject>

class Hyperion;

///
/// Singleton instance for simple api signal sharing across Hyperion instances
///
class ApiSync : public QObject
{
	Q_OBJECT
public:
    static ApiSync* getInstance()
    {
        static ApiSync instance;
        return & instance;
    }
private:
    ApiSync() {}

public:
    ApiSync(ApiSync const&)   = delete;
    void operator=(ApiSync const&)  = delete;

signals:
	//////
	/// FROM API to Hyperion
	//////
	void setColor(QObject* inst, const int &priority, const std::vector<ColorRgb>& ledColors, const int &timeout_ms, const QString& origin, const hyperion::Components& callerComp);

	void setEffect(QObject* inst, const QString& effectName, const int &priority, const int& duration, const QString& origin, const hyperion::Components& callerComp);

	void setSourceAutoSelect(QObject* inst, const bool state, const hyperion::Components& callerComp);

	void setVisiblePriority(QObject* inst, const int& priority, const hyperion::Components& callerComp);

	void clearPriority(QObject* inst, const int& priority,  const hyperion::Components& callerComp);

	void compStateChangeRequest(QObject* inst, const hyperion::Components& comp, const bool& state, const hyperion::Components& callerComp);

	void setLedMappingType(QObject* inst, const int& type, const hyperion::Components& callerComp);

	void setVideoMode(QObject* inst, const VideoMode& mode, const hyperion::Components& callerComp);

	void registerInput(QObject* inst, const int priority, const hyperion::Components& component, const QString& origin, const QString& owner, const hyperion::Components& callerComp);

	void setInputImage(QObject* inst, const int priority, const Image<ColorRgb>& image, const int64_t timeout_ms, const hyperion::Components& callerComp);

	void setInput(QObject* inst, const std::vector<ColorRgb>& ledColors, const int timeout_ms, const hyperion::Components& component);

	/// report back answer from requestPendingRegister call
	void answerActiveRegister(QObject* callerInstance, std::map<int,API::registerData> data);

	////
	// FROM Hyperion to API
	////
	void requestActiveRegister(QObject* callerInstance);


};
