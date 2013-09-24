package org.hyperion.config.spec;

import java.util.Observable;



/**
 * The LedFrame describes the construction of leds along the sides of the TV screen.
 *  
 */
public class LedFrameConstruction extends Observable {

	public enum Direction {
		clockwise,
		counter_clockwise;
	}
	
	/** True if the leds are organised clockwise else false (counter clockwise) */
	public boolean clockwiseDirection;
	
	/** True if the top left corner has a led else false */
	public boolean topLeftCorner;
	/** True if the top right corner has a led else false */
	public boolean topRightCorner;
	/** True if the bottom left corner has a led else false */
	public boolean bottomLeftCorner;
	/** True if the bottom right corner has a led else false */
	public boolean bottomRightCorner;
	
	/** The number of leds between the top-left corner and the top-right corner of the screen 
		(excluding the corner leds) */
	public int topLedCnt;
	/** The number of leds between the bottom-left corner and the bottom-right corner of the screen
		(excluding the corner leds) */
	public int bottomLedCnt;
	
	/** The number of leds between the top-left corner and the bottom-left corner of the screen
		(excluding the corner leds) */
	public int leftLedCnt;
	/** The number of leds between the top-right corner and the bottom-right corner of the screen
		(excluding the corner leds) */
	public int rightLedCnt;
	
	/** The offset (in leds) of the starting led counted clockwise from the top-left corner */
	public int firstLedOffset;
	
	/** The 'integration depth' of the leds along the horizontal axis of the tv */
	public double horizontalDepth = 0.05;
	/** The 'integration depth' of the leds along the vertical axis of the tv */
	public double verticalDepth   = 0.05;
	
	/** The fraction of overlap from one to another led */
	public double overlapFraction = 0.0;
	
	public int getLedCount() {
		int cornerLedCnt = 0;
		if (topLeftCorner)     ++cornerLedCnt;
		if (topRightCorner)    ++cornerLedCnt;
		if (bottomLeftCorner)  ++cornerLedCnt;
		if (bottomRightCorner) ++cornerLedCnt;
		
		return topLedCnt + bottomLedCnt + leftLedCnt + rightLedCnt + cornerLedCnt;
	}
	
	@Override
	public void setChanged() {
		super.setChanged();
	}
}
