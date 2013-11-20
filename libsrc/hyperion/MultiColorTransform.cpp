
// STL includes
#include <cassert>

// Hyperion includes
#include "MultiColorTransform.h"

MultiColorTransform::MultiColorTransform()
{
}

MultiColorTransform::~MultiColorTransform()
{
	// Clean up all the transforms
	for (ColorTransform * transform : _transform)
	{
		delete transform;
	}
}

void MultiColorTransform::addTransform(const std::string & id,
		const RgbChannelTransform & redTransform,
		const RgbChannelTransform & greenTransform,
		const RgbChannelTransform & blueTransform,
		const HsvTransform & hsvTransform)
{
	ColorTransform * transform = new ColorTransform();
	transform->_id = id;

	transform->_rgbRedTransform   = redTransform;
	transform->_rgbGreenTransform = greenTransform;
	transform->_rgbBlueTransform  = blueTransform;

	transform->_hsvTransform = hsvTransform;

	_transform.push_back(transform);
}

void MultiColorTransform::setTransformForLed(const std::string& id, const unsigned startLed, const unsigned endLed)
{
	assert(startLed <= endLed);

	// Make sure that there are at least enough led transforms to match the given indices
	if (_ledTransforms.size() < endLed+1)
	{
		_ledTransforms.resize(endLed+1, nullptr);
	}

	// Get the identified transform (don't care if is nullptr)
	ColorTransform * transform = getTransform(id);
	for (unsigned iLed=startLed; iLed<=endLed; ++iLed)
	{
		_ledTransforms[iLed] = transform;
	}
}

std::vector<std::string> MultiColorTransform::getTransformIds()
{
	// Create the list on the fly
	std::vector<std::string> transformIds;
	for (ColorTransform* transform : _transform)
	{
		transformIds.push_back(transform->_id);
	}
	return transformIds;
}

ColorTransform* MultiColorTransform::getTransform(const std::string& id)
{
	// Iterate through the unique transforms until we find the one with the given id
	for (ColorTransform* transform : _transform)
	{
		if (transform->_id == id)
		{
			return transform;
		}
	}

	// The ColorTransform was not found
	return nullptr;
}

std::vector<ColorRgb> MultiColorTransform::applyTransform(const std::vector<ColorRgb>& rawColors)
{
	// Create a copy, as we will do the rest of the transformation in place
	std::vector<ColorRgb> ledColors(rawColors);

	const size_t itCnt = std::min(_transform.size(), rawColors.size());
	for (size_t i=0; i<itCnt; ++i)
	{
		ColorTransform* transform = _ledTransforms[i];
		if (transform == nullptr)
		{
			// No transform set for this led (do nothing)
			continue;
		}
		ColorRgb& color = ledColors[i];

		transform->_hsvTransform.transform(color.red, color.green, color.blue);
		color.red   = transform->_rgbRedTransform.transform(color.red);
		color.green = transform->_rgbGreenTransform.transform(color.green);
		color.blue  = transform->_rgbBlueTransform.transform(color.blue);
	}
	return ledColors;
}
