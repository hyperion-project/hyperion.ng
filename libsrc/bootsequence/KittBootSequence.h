
#pragma once

// Bootsequence includes
#include "AbstractBootSequence.h"

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessor.h>


class KittBootSequence : public AbstractBootSequence
{
public:
	KittBootSequence(Hyperion * hyperion, const unsigned duration_ms);

	virtual ~KittBootSequence();

	virtual const std::vector<RgbColor>& nextColors();

private:
	/// Image processor to compute led-colors from the image
	ImageProcessor * _processor;

	/// 1D-Image of the KITT-grill contains a single red pixel and the rest black
	RgbImage _image;

	/// The vector with led-colors
	std::vector<RgbColor> _ledColors;

	bool _forwardMove = true;
	unsigned _currentLight = 0;

	void moveNextLight();
};

