#pragma once

#include <QColor>
#include <QImage>

#include <getoptPlusPlus/getoptpp.h>

struct ColorTransform
{
    double valueRed;
    double valueGreen;
    double valueBlue;
};

typedef vlofgren::PODParameter<QColor> ColorParameter;
typedef vlofgren::PODParameter<QImage> ImageParameter;
typedef vlofgren::PODParameter<ColorTransform> TransformParameter;

namespace vlofgren {
    template<>
    QColor ColorParameter::validate(const std::string& s) throw (Parameter::ParameterRejected)
    {
        // Check if we can create the color by name
        QColor color(s.c_str());
        if (color.isValid())
        {
            return color;
        }

        // check if we can create the color by hex RRGGBB value
        if (s.length() == 6 && isxdigit(s[0]) && isxdigit(s[1]) && isxdigit(s[2]) && isxdigit(s[3]) && isxdigit(s[4]) && isxdigit(s[5]))
        {
            bool ok = true;
            int rgb[3];
            for (int i = 0; i < 3 && ok; ++i)
            {
                QString colorComponent(s.substr(2*i, 2).c_str());
                rgb[i] = colorComponent.toInt(&ok, 16);
            }

            // check if all components parsed succesfully
            if (ok)
            {
                color.setRgb(rgb[0], rgb[1], rgb[2]);
                return color;
            }
        }

        std::stringstream errorMessage;
        errorMessage << "Invalid color. A color is specified by a six lettered RRGGBB hex value or one of the following names:";
        foreach (const QString & colorname, QColor::colorNames()) {
            errorMessage << "\n  " << colorname.toStdString();
        }
        throw Parameter::ParameterRejected(errorMessage.str());

        return color;
    }

    template<>
    QImage ImageParameter::validate(const std::string& s) throw (Parameter::ParameterRejected)
    {
        QImage image(s.c_str());

        if (image.isNull())
        {
            std::stringstream errorMessage;
            errorMessage << "File " << s << " could not be opened as an image";
            throw Parameter::ParameterRejected(errorMessage.str());
        }

        return image;
    }

    template<>
    ColorTransform TransformParameter::validate(const std::string& s) throw (Parameter::ParameterRejected)
    {
        ColorTransform transform;

        // s should be split in 3 parts
        // seperators are either a ',' or a space
        QStringList components = QString(s.c_str()).split(" ", QString::SkipEmptyParts);

        if (components.size() == 3)
        {
            bool ok1, ok2, ok3;
            transform.valueRed   = components[0].toDouble(&ok1);
            transform.valueGreen = components[1].toDouble(&ok2);
            transform.valueBlue  = components[2].toDouble(&ok3);

            if (ok1 && ok2 && ok3)
            {
                return transform;
            }
        }

        std::stringstream errorMessage;
        errorMessage << "Argument " << s << " can not be parsed to 3 double values";
        throw Parameter::ParameterRejected(errorMessage.str());

        return transform;
    }
}
