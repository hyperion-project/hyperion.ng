#ifndef JSONINFO_H
#define JSONINFO_H

#include <utils/Logger.h>
#include <hyperion/Hyperion.h>
#include <hyperion/HyperionIManager.h>

#include <QJsonObject>
#include <QJsonArray>
#include <QSharedPointer>


class JsonInfo
{

public:
	static QJsonObject getInfo(const QSharedPointer<Hyperion>& hyperionInstance, QSharedPointer<Logger> log);
	static QJsonArray getAdjustmentInfo(const QSharedPointer<Hyperion>& hyperionInstance, QSharedPointer<Logger> log);
	static QJsonArray getPrioritiestInfo(const QSharedPointer<Hyperion>& hyperionInstance);
	static QJsonArray getPrioritiestInfo(int currentPriority, const PriorityMuxer::InputsMap& activeInputs);
	static QJsonArray getEffects();
	static QJsonArray getEffectSchemas();
	static QJsonArray getAvailableScreenGrabbers();
	static QJsonArray getAvailableVideoGrabbers();
	static QJsonArray getAvailableAudioGrabbers();
	static QJsonObject getGrabbers(const QSharedPointer<Hyperion>& hyperionInstance);
	static QJsonObject getAvailableLedDevices();
	static QJsonObject getCecInfo();
	static QJsonArray getServices();
	static QJsonArray getComponents(const QSharedPointer<Hyperion>& hyperionInstance);
	static QJsonArray getInstanceInfo();
	static QJsonArray getActiveEffects(const QSharedPointer<Hyperion>& hyperionInstance);
	static QJsonArray getActiveColors(const QSharedPointer<Hyperion>& hyperionInstance);
	static QJsonArray getTransformationInfo(const QSharedPointer<Hyperion>& hyperionInstance);
	static QJsonObject getSystemInfo();
	QJsonObject discoverSources (const QString& sourceType, const QJsonObject& params);

	static QJsonObject getConfiguration(const QList<quint8>& instanceIds = {}, const QStringList& instanceFilteredTypes = {}, const QStringList& globalFilterTypes = {} );

private:

	template<typename GrabberType>
	void discoverGrabber(QJsonArray& inputs, const QJsonObject& params) const;
	QJsonArray discoverScreenInputs(const QJsonObject& params) const;
	QJsonArray discoverVideoInputs(const QJsonObject& params) const;
	QJsonArray discoverAudioInputs(const QJsonObject& params) const;
};

#endif // JSONINFO_H
