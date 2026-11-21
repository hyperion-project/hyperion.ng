#include "V4L2ControlOptions.h"

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

		// toInt() returns the fallback unless the value is a whole number that fits into an int,
		// so differing results for two fallbacks identify an invalid value.
		const int parsed = value.toInt(0);
		if (parsed != value.toInt(1))
		{
			return QStringLiteral("Device %1 has an invalid default for --%2").arg(device, control.name);
		}

		control.value = parsed;
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
