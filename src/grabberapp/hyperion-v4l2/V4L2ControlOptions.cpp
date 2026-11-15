#include "V4L2ControlOptions.h"

#include <cmath>
#include <limits>

QString applyV4L2ControlDefaults(V4L2ControlOptions& options, const QJsonObject& defaults, const QString& device)
{
	const struct
	{
		QString name;
		std::optional<int>& value;
	} controls[] = {
		{"brightness", options.brightness},
		{"contrast", options.contrast},
		{"saturation", options.saturation},
		{"hue", options.hue}
	};

	for (const auto& control : controls)
	{
		const QJsonValue value = defaults.value(control.name);
		if (control.value.has_value() || value.isUndefined())
		{
			continue;
		}

		if (!value.isDouble() || !std::isfinite(value.toDouble()) ||
		    value.toDouble() < std::numeric_limits<int>::min() || value.toDouble() > std::numeric_limits<int>::max() ||
		    std::trunc(value.toDouble()) != value.toDouble())
		{
			return QStringLiteral("Device %1 has an invalid default for --%2").arg(device, control.name);
		}

		control.value = static_cast<int>(value.toDouble());
	}

	return QString();
}

QString validateV4L2ControlOptions(const V4L2ControlOptions& options, const QJsonObject& properties, const QString& device)
{
	const struct
	{
		QString name;
		const std::optional<int>& value;
	} controls[] = {
		{"brightness", options.brightness},
		{"contrast", options.contrast},
		{"saturation", options.saturation},
		{"hue", options.hue}
	};

	for (const auto& control : controls)
	{
		if (!control.value.has_value())
		{
			continue;
		}

		const QJsonValue property = properties.value(control.name);
		if (!property.isObject())
		{
			return QStringLiteral("Device %1 does not support --%2").arg(device, control.name);
		}

		const QJsonObject range = property.toObject();
		const QJsonValue min = range.value(QStringLiteral("minValue"));
		const QJsonValue max = range.value(QStringLiteral("maxValue"));
		if (!min.isDouble() || !max.isDouble() || min.toDouble() > max.toDouble())
		{
			return QStringLiteral("Device %1 has an invalid range for --%2").arg(device, control.name);
		}
		if (*control.value < min.toDouble() || *control.value > max.toDouble())
		{
			return QStringLiteral("Value %1 for --%2 is outside the range %3 to %4 for device %5")
				.arg(*control.value).arg(control.name).arg(min.toDouble()).arg(max.toDouble()).arg(device);
		}
	}

	return QString();
}
