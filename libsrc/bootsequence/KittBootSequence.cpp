
// Hyperion includes
#include <hyperion/ImageProcessorFactory.h>

// Local-Bootsequence includes
#include "KittBootSequence.h"

KittBootSequence::KittBootSequence(Hyperion * hyperion, const unsigned duration_ms) :
	AbstractBootSequence(hyperion, 100, duration_ms/100),
	_processor(ImageProcessorFactory::getInstance().newImageProcessor()),
	_image(9, 1, ColorRgb{0,0,0}),
	_ledColors(hyperion->getLedCount(), ColorRgb{0,0,0}),
	_forwardMove(false),
	_currentLight(0)
{
	// empty
}

KittBootSequence::~KittBootSequence()
{
	delete _processor;
}

const std::vector<ColorRgb>& KittBootSequence::nextColors()
{

	// Switch the previous light 'off'
	_image(_currentLight, 0) = ColorRgb{0,0,0};

	// Move the current to the next light
	moveNextLight();

	// Switch the current light 'on'
	_image(_currentLight, 0) = ColorRgb{255,0,0};


	// Translate the 'image' to led colors
	_processor->process(_image, _ledColors);

	// Return the colors
	return _ledColors;
}

void KittBootSequence::moveNextLight()
{
	// Increase/Decrease the current light
	if (_forwardMove)
	{
		++_currentLight;
		if (_currentLight == _image.width())
		{
			_forwardMove = false;
			--_currentLight;
		}
	}
	else
	{
		if (_currentLight == 0)
		{
			_forwardMove = true;
		}
		else
		{
			--_currentLight;
		}
	}
}
