#ifndef V4L2GRABBERDEBUG_H
#define V4L2GRABBERDEBUG_H

#include <QDebug>
#include <QMap>
#include <QList>
#include "grabber/video/v4l2/V4L2Grabber.h"

namespace V4L2GrabberDebug {

// ------------------ Enums ------------------
inline QDebug operator<<(QDebug dbg, const PixelFormat &fmt)
{
	dbg.nospace() << pixelFormatToString(fmt);
	return dbg.space();
}

inline QDebug operator<<(QDebug dbg, const VideoStandard &std)
{
	dbg.nospace() << VideoStandard2String(std);
	return dbg.space();
}

// ------------------ EncodingProperties ------------------
inline QDebug operator<<(QDebug dbg, const V4L2Grabber::DeviceProperties::InputProperties::EncodingProperties &enc)
{
	dbg.nospace() << "{width:" << enc.width
				  << ", height:" << enc.height
				  << ", framerates:[";
	for (int i = 0; i < enc.framerates.size(); ++i)
	{
		dbg.nospace() << enc.framerates[i];
		if (i != enc.framerates.size() - 1) dbg.nospace() << ", ";
	}
	dbg.nospace() << "]}";
	return dbg.space();
}

// ------------------ InputProperties ------------------
inline QDebug operator<<(QDebug dbg, const V4L2Grabber::DeviceProperties::InputProperties &input)
{
	dbg.nospace() << "{inputName:" << input.inputName
				  << ", standards:[";
	for (int i = 0; i < input.standards.size(); ++i)
	{
		dbg.nospace() << input.standards[i];
		if (i != input.standards.size() - 1) dbg.nospace() << ", ";
	}
	dbg.nospace() << "], encodingFormats:{\n";

	int count = 0;
	int size = input.encodingFormats.size();
	for (auto it = input.encodingFormats.constBegin(); it != input.encodingFormats.constEnd(); ++it, ++count)
	{
		dbg.nospace() << "    " << it.key() << ": " << it.value();
		if (count != size - 1) dbg.nospace() << ",\n";
	}

	dbg.nospace() << "\n}}";
	return dbg.space();
}

// ------------------ DeviceProperties ------------------
inline QDebug operator<<(QDebug dbg, const V4L2Grabber::DeviceProperties &dev)
{
	dbg.nospace() << "{name:" << dev.name << ", inputs:{\n";

	int count = 0;
	int size = dev.inputs.size();
	for (auto it = dev.inputs.constBegin(); it != dev.inputs.constEnd(); ++it, ++count)
	{
		dbg.nospace() << "  " << it.key() << ": " << it.value();
		if (count != size - 1) dbg.nospace() << ",\n";
	}

	dbg.nospace() << "\n}}";
	return dbg.space();
}

// ------------------ DeviceControls ------------------
inline QDebug operator<<(QDebug dbg, const V4L2Grabber::DeviceControls &ctrl)
{
	dbg.nospace() << "{property:" << ctrl.property
				  << ", min:" << ctrl.minValue
				  << ", max:" << ctrl.maxValue
				  << ", step:" << ctrl.step
				  << ", default:" << ctrl.defaultValue
				  << ", current:" << ctrl.currentValue
				  << "}";
	return dbg.space();
}

// ------------------ QList<DeviceControls> ------------------
inline QDebug operator<<(QDebug dbg, const QList<V4L2Grabber::DeviceControls> &list)
{
	dbg.nospace() << "[\n";
	for (int i = 0; i < list.size(); ++i)
	{
		dbg.nospace() << "  " << list[i];
		if (i != list.size() - 1) dbg.nospace() << ",\n";
	}
	dbg.nospace() << "\n]";
	return dbg.space();
}

// ------------------ QMap<QString, DeviceProperties> ------------------
inline QDebug operator<<(QDebug dbg, const QMap<QString, V4L2Grabber::DeviceProperties> &map)
{
	dbg.nospace() << "{\n";
	int count = 0;
	int size = map.size();
	for (auto it = map.constBegin(); it != map.constEnd(); ++it, ++count)
	{
		dbg.nospace() << "  \"" << it.key() << "\": " << it.value();
		if (count != size - 1) dbg.nospace() << ",\n";
	}
	dbg.nospace() << "\n}";
	return dbg.space();
}

// ------------------ QMap<QString, QList<DeviceControls>> ------------------
inline QDebug operator<<(QDebug dbg, const QMap<QString, QList<V4L2Grabber::DeviceControls>> &map)
{
	dbg.nospace() << "{\n";
	int count = 0;
	int size = map.size();
	for (auto it = map.constBegin(); it != map.constEnd(); ++it, ++count)
	{
		dbg.nospace() << "  \"" << it.key() << "\": " << it.value();
		if (count != size - 1) dbg.nospace() << ",\n";
	}
	dbg.nospace() << "\n}";
	return dbg.space();
}

} // namespace V4L2GrabberDebug

#endif // V4L2GRABBERDEBUG_H
