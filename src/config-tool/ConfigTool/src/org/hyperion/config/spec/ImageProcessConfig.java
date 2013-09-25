package org.hyperion.config.spec;

import java.util.Observable;

public class ImageProcessConfig extends Observable {
	
	/** The 'integration depth' of the leds along the horizontal axis of the tv */
	public double horizontalDepth = 0.05;
	/** The 'integration depth' of the leds along the vertical axis of the tv */
	public double verticalDepth   = 0.05;
	
	/** The fraction of overlap from one to another led */
	public double overlapFraction = 0.0;
	
	public boolean blackBorderRemoval = true;
	
	@Override
	public void setChanged() {
		super.setChanged();
	}
}
