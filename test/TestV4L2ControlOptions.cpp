#include "../src/grabberapp/hyperion-v4l2/V4L2ControlOptions.h"

#include <iostream>

int main()
{
	const QJsonObject properties{
		{QStringLiteral("brightness"), QJsonObject{{QStringLiteral("minValue"), 0}, {QStringLiteral("maxValue"), 255}, {QStringLiteral("current"), 128}}},
		{QStringLiteral("contrast"), QJsonObject{{QStringLiteral("minValue"), 0}, {QStringLiteral("maxValue"), 255}, {QStringLiteral("current"), 128}}},
		{QStringLiteral("saturation"), QJsonObject{{QStringLiteral("minValue"), 0}, {QStringLiteral("maxValue"), 255}, {QStringLiteral("current"), 128}}},
		{QStringLiteral("hue"), QJsonObject{{QStringLiteral("minValue"), -180}, {QStringLiteral("maxValue"), 180}, {QStringLiteral("current"), 0}}}
	};
	const QString device = QStringLiteral("/dev/video0");
	auto check = [](bool condition, const char* message) {
		if (!condition)
			std::cerr << message << '\n';
		return condition;
	};
	bool ok = true;
	V4L2ControlOptions options;
	ok &= check(validateV4L2ControlOptions(options, QJsonObject(), device).isEmpty(), "Unspecified controls must not require device properties");
	options.brightness = 0;
	options.contrast = 255;
	options.saturation = 128;
	options.hue = -180;
	ok &= check(validateV4L2ControlOptions(options, properties, device).isEmpty(), "Valid values including range boundaries must be accepted");
	options.hue = 180;
	ok &= check(validateV4L2ControlOptions(options, properties, device).isEmpty(), "Hue upper boundary must be accepted");

	for (const auto& test : {
		std::pair<std::optional<int>*, const char*>{&options.brightness, "brightness"},
		{&options.contrast, "contrast"},
		{&options.saturation, "saturation"},
		{&options.hue, "hue"}
	})
	{
		const auto previous = *test.first;
		*test.first = -181;
		QString error = validateV4L2ControlOptions(options, properties, device);
		ok &= check(error.contains(QLatin1String(test.second)) && error.contains(device), "Below-range value must identify its control and device");
		*test.first = 256;
		error = validateV4L2ControlOptions(options, properties, device);
		ok &= check(error.contains(QLatin1String(test.second)), "Above-range value must identify its control");
		*test.first = previous;
	}

	QJsonObject missing = properties;
	missing.remove(QStringLiteral("hue"));
	ok &= check(validateV4L2ControlOptions(options, missing, device).contains(QStringLiteral("--hue")), "Unsupported explicit control must be rejected");
	options.hue.reset();
	ok &= check(validateV4L2ControlOptions(options, missing, device).isEmpty(), "Unsupported unspecified control must be ignored");
	missing[QStringLiteral("brightness")] = QJsonObject{{QStringLiteral("minValue"), 0}};
	ok &= check(validateV4L2ControlOptions(options, missing, device).contains(QStringLiteral("invalid range")), "Incomplete range must be rejected");
	missing[QStringLiteral("brightness")] = QJsonObject{{QStringLiteral("minValue"), 10}, {QStringLiteral("maxValue"), 0}};
	ok &= check(validateV4L2ControlOptions(options, missing, device).contains(QStringLiteral("invalid range")), "Reversed range must be rejected");

	const QJsonObject defaults{
		{QStringLiteral("brightness"), 128},
		{QStringLiteral("contrast"), 64},
		{QStringLiteral("saturation"), 0},
		{QStringLiteral("hue"), -10}
	};
	V4L2ControlOptions selected;
	selected.brightness = 0;
	ok &= check(applyV4L2ControlDefaults(selected, defaults, device).isEmpty(), "Valid defaults must be accepted");
	ok &= check(selected.brightness == 0 && selected.contrast == 64 && selected.saturation == 0 && selected.hue == -10,
	            "Explicit zero must override defaults and other defaults must be applied");
	ok &= check(validateV4L2ControlOptions(selected, properties, device).isEmpty(), "Resolved defaults must pass range validation");

	V4L2ControlOptions partial;
	ok &= check(applyV4L2ControlDefaults(partial, QJsonObject{{QStringLiteral("brightness"), 42}}, device).isEmpty(),
	            "Missing defaults must be allowed");
	ok &= check(partial.brightness == 42 && !partial.contrast && !partial.saturation && !partial.hue,
	            "Controls without defaults must remain unchanged");
	V4L2ControlOptions invalid;
	ok &= check(applyV4L2ControlDefaults(invalid, QJsonObject{{QStringLiteral("hue"), QStringLiteral("bad")}}, device).contains(QStringLiteral("--hue")),
	            "Non-numeric defaults must be rejected");
	ok &= check(applyV4L2ControlDefaults(invalid, QJsonObject{{QStringLiteral("hue"), 1.5}}, device).contains(QStringLiteral("--hue")),
	            "Fractional defaults must be rejected");
	invalid.hue = 0;
	ok &= check(applyV4L2ControlDefaults(invalid, QJsonObject{{QStringLiteral("hue"), QStringLiteral("bad")}}, device).isEmpty(),
	            "Explicit control values must override even malformed defaults");
	invalid.hue.reset();
	ok &= check(applyV4L2ControlDefaults(invalid, QJsonObject{{QStringLiteral("hue"), 181}}, device).isEmpty() &&
	            validateV4L2ControlOptions(invalid, properties, device).contains(QStringLiteral("--hue")),
	            "Out-of-range defaults must be rejected by range validation");
	return ok ? 0 : 1;
}
