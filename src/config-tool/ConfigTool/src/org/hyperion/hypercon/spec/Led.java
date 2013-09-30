package org.hyperion.hypercon.spec;

import java.awt.geom.Point2D;
import java.awt.geom.Rectangle2D;


/**
 * Led specification with fractional location along screen border and fractional-rectangle for 
 * integrating an image into led color  
 */
public class Led {
	/** The sequence number of the led */
	public int mLedSeqNr;
	
	/** The side along which the led is placed */
	public BorderSide mSide;
	
	/** The fractional location of the led */
	public Point2D mLocation;

	/** The fractional rectangle for image integration */
	public Rectangle2D mImageRectangle;
	
	/**
	 * String representation of the led specification
	 * 
	 * @return The led specs as nice readable string
	 */
	@Override
	public String toString() {
		return "Led[" + mLedSeqNr + "] Location=" + mLocation + " Rectangle=" + mImageRectangle;
	}
}
