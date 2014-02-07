#pragma once

// STL includes
#include <algorithm>

// Qt includes
#include <QColor>
#include <QImage>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

// hyperion-remote includes
#include "ColorTransformValues.h"

/// Data parameter for a color
typedef vlofgren::PODParameter<std::vector<QColor>> ColorParameter;

/// Data parameter for an image
typedef vlofgren::PODParameter<QImage> ImageParameter;

/// Data parameter for color transform values (list of three values)
typedef vlofgren::PODParameter<ColorTransformValues> TransformParameter;

namespace vlofgren {
	///
	/// Translates a string (as passed on the commandline) to a vector of colors
	///
	/// @param[in] s The string (as passed on the commandline)
	///
	/// @return The translated colors
	///
	/// @throws Parameter::ParameterRejected If the string did not result in a color
	///
	template<>
	std::vector<QColor> ColorParameter::validate(const std::string& s) throw (Parameter::ParameterRejected)
	{
		// Check if we can create the color by name
		QColor color(s.c_str());
		if (color.isValid())
		{
			return std::vector<QColor>{color};
		}

		// check if we can create the color by hex RRGGBB value
		if (s.length() >= 6u && (s.length()%6u) == 0u && std::count_if(s.begin(), s.end(), isxdigit) == int(s.length()))
		{
			bool ok = true;
			std::vector<QColor> colors;

			for (size_t j = 0; j < s.length()/6; ++j)
			{
				int rgb[3];
				for (int i = 0; i < 3 && ok; ++i)
				{
					QString colorComponent(s.substr(6*j+2*i, 2).c_str());
					rgb[i] = colorComponent.toInt(&ok, 16);
				}

				if (ok)
				{
					color.setRgb(rgb[0], rgb[1], rgb[2]);
					colors.push_back(color);
				}
				else
				{
					break;
				}
			}

			// check if all components parsed succesfully
			if (ok)
			{
				return colors;
			}
		}

		std::stringstream errorMessage;
		errorMessage << "Invalid color. A color is specified by a six lettered RRGGBB hex value or one of the following names:";
		foreach (const QString & colorname, QColor::colorNames()) {
			errorMessage << "\n  " << colorname.toStdString();
		}
		throw Parameter::ParameterRejected(errorMessage.str());

		return std::vector<QColor>{color};
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
	ColorTransformValues TransformParameter::validate(const std::string& s) throw (Parameter::ParameterRejected)
	{
		ColorTransformValues transform;

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
