#pragma once

// STL includes
#include <cstdint>

/// Correction for a single color byte value
/// All configuration values are unsigned int and assume the color value to be between 0 and 255
class RgbChannelAdjustment
{
public:
	/// Default constructor
	RgbChannelAdjustment();
	
	/// Constructor
	/// @param adjustR  
	/// @param adjustG 
	/// @param adjustB 

	RgbChannelAdjustment(int adjustR, int adjustG, int adjustB);

	/// Destructor
	~RgbChannelAdjustment();

	/// @return The current adjustR value
	uint8_t getadjustmentR() const;

	/// @param threshold New adjustR value
	void setadjustmentR(uint8_t adjustR);

	/// @return The current adjustG value
	uint8_t getadjustmentG() const;

	/// @param gamma New adjustG value
	void setadjustmentG(uint8_t adjustG);

	/// @return The current adjustB value
	uint8_t getadjustmentB() const;

	/// @param blacklevel New adjustB value
	void setadjustmentB(uint8_t adjustB);

	/// Transform the given array value
	/// @param input The input color bytes
	/// @return The corrected byte value
	uint8_t adjustmentR(uint8_t inputR) const;
	uint8_t adjustmentG(uint8_t inputG) const;
	uint8_t adjustmentB(uint8_t inputB) const;


private:
	/// (re)-initilize the color mapping
	void initializeMapping();

private:
	/// The adjustment of R channel
	int _adjustR;
	/// The adjustment of G channel
	int _adjustG;
	/// The adjustment of B channel
	int _adjustB;
	
	/// The mapping from input color to output color
	int _mappingR[256];
	int _mappingG[256];
	int _mappingB[256];
};
