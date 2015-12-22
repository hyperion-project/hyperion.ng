#pragma once

// STL includes
#include <cstdint>

/// Correction for a single color byte value
/// All configuration values are unsigned int and assume the color value to be between 0 and 255
class RgbChannelCorrection
{
public:
	/// Default constructor
	RgbChannelCorrection();
	
	/// Constructor
	/// @param correctionR  
	/// @param correctionG 
	/// @param correctionB 

	RgbChannelCorrection(int correctionR, int correctionG, int correctionB);

	/// Destructor
	~RgbChannelCorrection();

	/// @return The current correctionR value
	uint8_t getcorrectionR() const;

	/// @param threshold New correctionR value
	void setcorrectionR(uint8_t correctionR);

	/// @return The current correctionG value
	uint8_t getcorrectionG() const;

	/// @param gamma New correctionG value
	void setcorrectionG(uint8_t correctionG);

	/// @return The current correctionB value
	uint8_t getcorrectionB() const;

	/// @param blacklevel New correctionB value
	void setcorrectionB(uint8_t correctionB);

	/// Transform the given array value
	/// @param input The input color bytes
	/// @return The corrected byte value
	uint8_t correctionR(uint8_t inputR) const;
	uint8_t correctionG(uint8_t inputG) const;
	uint8_t correctionB(uint8_t inputB) const;


private:
	/// (re)-initilize the color mapping
	void initializeMapping();

private:
	/// The correction of R channel
	int _correctionR;
	/// The correction of G channel
	int _correctionG;
	/// The correction of B channel
	int _correctionB;
	
	/// The mapping from input color to output color
	int _mappingR[256];
	int _mappingG[256];
	int _mappingB[256];
};
