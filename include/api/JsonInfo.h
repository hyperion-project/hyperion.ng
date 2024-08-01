#ifndef JSONINFO_H
#define JSONINFO_H

#include <utils/Logger.h>
#include <hyperion/Hyperion.h>
#include <hyperion/HyperionIManager.h>

#include <QJsonObject>
#include <QJsonArray>

class JsonInfo
{

public:
	static QJsonArray getAdjustmentInfo(const Hyperion* hyperion, Logger* log);
	static QJsonArray getPrioritiestInfo(const Hyperion* hyperion);
	static QJsonArray getPrioritiestInfo(int currentPriority, const PriorityMuxer::InputsMap& activeInputs);
	static QJsonArray getEffects(const Hyperion* hyperion);
	static QJsonArray getAvailableScreenGrabbers();
	static QJsonArray getAvailableVideoGrabbers();
	static QJsonArray getAvailableAudioGrabbers();
	static QJsonObject getGrabbers(const Hyperion* hyperion);
	static QJsonObject getAvailableLedDevices();
	static QJsonObject getCecInfo();
	static QJsonArray getServices();
	static QJsonArray getComponents(const Hyperion* hyperion);
	static QJsonArray getInstanceInfo();
	static QJsonArray getActiveEffects(const Hyperion* hyperion);
	static QJsonArray getActiveColors(const Hyperion* hyperion);
	static QJsonArray getTransformationInfo(const Hyperion* hyperion);
	static QJsonObject getSystemInfo(const Hyperion* hyperion);
	QJsonObject discoverSources (const QString& sourceType, const QJsonObject& params);

	static QJsonObject getConfiguration(const QList<quint8>& instances = {}, bool addGlobalConfig = true, const QStringList& instanceFilteredTypes = {}, const QStringList& globalFilterTypes = {} );

private:

	template<typename GrabberType>
	void discoverGrabber(QJsonArray& inputs, const QJsonObject& params) const;
	QJsonArray discoverScreenInputs(const QJsonObject& params) const;
	QJsonArray discoverVideoInputs(const QJsonObject& params) const;
	QJsonArray discoverAudioInputs(const QJsonObject& params) const;
};

#endif // JSONINFO_H
