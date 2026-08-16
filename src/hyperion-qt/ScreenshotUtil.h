#pragma once

#include <QString>

#include <utils/ColorRgb.h>
#include <utils/Image.h>

void saveScreenshot(const QString& filename, const Image<ColorRgb>& image);
