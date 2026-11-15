#pragma once

#include <optional>

#include <QJsonObject>
#include <QString>

struct V4L2ControlOptions
{
	std::optional<int> brightness;
	std::optional<int> contrast;
	std::optional<int> saturation;
	std::optional<int> hue;
};

QString validateV4L2ControlOptions(const V4L2ControlOptions& options, const QJsonObject& properties, const QString& device);
QString applyV4L2ControlDefaults(V4L2ControlOptions& options, const QJsonObject& defaults, const QString& device);
