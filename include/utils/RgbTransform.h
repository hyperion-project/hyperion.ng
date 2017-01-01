#pragma once

// STL includes
#include <cstdint>

///
/// Color transformation to adjust the saturation and value of a RGB color value
///
class RgbTransform
{
public:
	///
	/// Default constructor
	///
	RgbTransform();

	///
	/// Constructor
	///
	/// @param saturationGain The used saturation gain
	/// @param valueGain The used value gain
	///
// 	HsvTransform(double saturationGain, double valueGain);

	///
	/// Destructor
	///
	~RgbTransform();

	/// @return The current red gamma value
	double getGammaR() const;

	/// @return The current green gamma value
	double getGammaG() const;

	/// @return The current blue gamma value
	double getGammaB() const;

	/// @param gamma New gamma value
	void setGamma(double gammaR,double gammaG=-1, double gammaB=-1);

	/// @return The current lower threshold
	double getThresholdLow() const;

	/// @param gamma New lower threshold
	void setThresholdLow(double threshold);

	/// @return The current lower threshold
	double getThresholdHigh() const;

	/// @param gamma New lower threshold
	void setThresholdHigh(double threshold);

	///
	/// Apply the transform the the given RGB values.
	///
	/// @param red The red color component
	/// @param green The green color component
	/// @param blue The blue color component
	///
	/// @note The values are updated in place.
	///
	void transform(uint8_t & red, uint8_t & green, uint8_t & blue);

private:
	/// (re)-initilize the color mapping
	void initializeMapping();	/// The saturation gain

	uint8_t _thresholdLow;
	uint8_t _thresholdHigh;

	double _gammaR;
	double _gammaG;
	double _gammaB;
	
	/// The mapping from input color to output color
	uint8_t _mappingR[256];
	uint8_t _mappingG[256];
	uint8_t _mappingB[256];
};
