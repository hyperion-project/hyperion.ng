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
	RgbChannelAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB);

	/// Destructor
	~RgbChannelAdjustment();

	/// setAdjustment RGB
	/// @param adjustR  
	/// @param adjustG 
	/// @param adjustB 
	void setAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB);

	/// @return The current adjustR value
	uint8_t getAdjustmentR() const;

	/// @param threshold New adjustR value
	void setAdjustmentR(uint8_t adjustR);

	/// @return The current adjustG value
	uint8_t getAdjustmentG() const;

	/// @param gamma New adjustG value
	void setAdjustmentG(uint8_t adjustG);

	/// @return The current adjustB value
	uint8_t getAdjustmentB() const;

	/// @param blacklevel New adjustB value
	void setAdjustmentB(uint8_t adjustB);

	/// Transform the given array value
	/// @param input The input color bytes
	/// @return The corrected byte value
	uint8_t getAdjustmentR(uint8_t inputR) const;
	uint8_t getAdjustmentG(uint8_t inputG) const;
	uint8_t getAdjustmentB(uint8_t inputB) const;


private:
	/// color channels
	enum ColorChannel { RED=0,GREEN=1, BLUE=2 };

	/// (re)-initilize the color mapping
	void initializeMapping();
	
	/// The adjustment of RGB channel
	uint8_t _adjust[3];
	
	/// The mapping from input color to output color
	uint8_t _mapping[3][256];
};
