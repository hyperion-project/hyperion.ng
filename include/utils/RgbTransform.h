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
	/// @param gammaR The used red gamma
	/// @param gammaG The used green gamma
	/// @param gammab The used blue gamma
	/// @param backlightThreshold The used lower brightness
	/// @param backlightColored use color in backlight
	/// @param brightnessHigh The used higher brightness
	///
	RgbTransform(double gammaR, double gammaG, double gammaB, double backlightThreshold, bool backlightColored, double brightnessHigh);

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

	/// @param gammaR New red gamma value
	/// @param gammaG New green gamma value
	/// @param gammaB New blue gamma value
	void setGamma(double gammaR,double gammaG=-1, double gammaB=-1);

	/// @return The current lower brightness
	double getBacklightThreshold() const;

	/// @param backlightThreshold New lower brightness
	void setBacklightThreshold(double backlightThreshold);

	/// @return The current state
	bool getBacklightColored() const;

	/// @param backlightColored en/disable colored backlight
	void setBacklightColored(bool backlightColored);

	/// @return The current brightness
	double getBrightness() const;

	/// @param brightness New brightness
	void setBrightness(double brightness);

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
	///
	/// init
	///
	/// @param gammaR The used red gamma
	/// @param gammaG The used green gamma
	/// @param gammab The used blue gamma
	/// @param backlightThreshold The used lower brightness
	/// @param backlightColored en/disable color in backlight
	/// @param brightnessHigh The used higher brightness
	///
	void init(double gammaR, double gammaG, double gammaB, double backlightThreshold, bool backlightColored, double brightnessHigh);

	/// (re)-initilize the color mapping
	void initializeMapping();	/// The saturation gain

	double  _backlightThreshold;
	bool    _backlightColored;
	double  _brightnessHigh;
	double  _sumBrightnessLow;
	double  _sumBrightnessHigh;

	double _gammaR;
	double _gammaG;
	double _gammaB;
	
	/// The mapping from input color to output color
	uint8_t _mappingR[256];
	uint8_t _mappingG[256];
	uint8_t _mappingB[256];
};
