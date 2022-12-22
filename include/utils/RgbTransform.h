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
	RgbTransform(double gammaR, double gammaG, double gammaB, double backlightThreshold, bool backlightColored, uint8_t brightness, uint8_t brightnessCompensation);

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
	int getBacklightThreshold() const;

	/// @param backlightThreshold New lower brightness
	void setBacklightThreshold(double backlightThreshold);

	/// @return The current state
	bool getBacklightColored() const;

	/// @param backlightColored en/disable colored backlight
	void setBacklightColored(bool backlightColored);

	/// @return return state of backlight
	bool getBackLightEnabled() const;

	/// @param enable en/disable backlight
	void setBackLightEnabled(bool enable);

	/// @return The current brightness
	uint8_t getBrightness() const;

	/// @param brightness New brightness
	void setBrightness(uint8_t brightness);

	/// @return The current brightnessCompensation value
	uint8_t getBrightnessCompensation() const;

	/// @param brightnessCompensation new brightnessCompensation
	void setBrightnessCompensation(uint8_t brightnessCompensation);

	///
	/// get component values of brightness for compensated brightness
	///
	///	@param rgb the rgb component
	///	@param cmy the cyan magenta yellow component
	///	@param w the white component
	///
	/// @note The values are updated in place.
	///
	void getBrightnessComponents(uint8_t & rgb, uint8_t & cmy, uint8_t & w) const;

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
	/// @param brightness The used brightness
	/// @param brightnessCompensation The used brightness compensation
	///
	void init(double gammaR, double gammaG, double gammaB, double backlightThreshold, bool backlightColored, uint8_t brightness, uint8_t brightnessCompensation);

	/// (re)-initilize the color mapping
	void initializeMapping();	/// The saturation gain

	void updateBrightnessComponents();

	/// backlight variables
	bool      _backLightEnabled
		, _backlightColored;
	double    _backlightThreshold
		, _sumBrightnessLow;

	/// gamma variables
	double    _gammaR
		, _gammaG
		, _gammaB;

	/// The mapping from input color to output color
	uint8_t   _mappingR[256]
		, _mappingG[256]
		, _mappingB[256];

	/// brightness variables
	uint8_t   _brightness
		, _brightnessCompensation
		, _brightness_rgb
		, _brightness_cmy
		, _brightness_w;
};
