package org.hyperion.config.spec;

import java.awt.geom.Point2D;
import java.awt.geom.Rectangle2D;


public class Led {

	public int mLedSeqNr;
	
	public BorderSide mSide;
	
	public Point2D mLocation;

	public Rectangle2D mImageRectangle;
	
	@Override
	public String toString() {
		return "Led[" + mLedSeqNr + "] Location=" + mLocation + " Rectangle=" + mImageRectangle;
	}
}
