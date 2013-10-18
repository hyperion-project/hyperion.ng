package org.hyperion.hypercon.spec;

import java.util.Observable;

import org.hyperion.hypercon.LedFrameFactory;

/**
 * Configuration parameters for the image processing. These settings are translated using the 
 * {@link LedFrameFactory} to configuration items used in the Hyperion daemon configfile.
 *
 */
public class ImageProcessConfig extends Observable {
	
	/** The 'integration depth' of the leds along the horizontal axis of the tv */
	private double mHorizontalDepth = 0.08;
	/** The 'integration depth' of the leds along the vertical axis of the tv */
	private double mVerticalDepth   = 0.05;
	
	/** The gap between the border integration area for the horizontal leds */
	private double mHorizontalGap = 0.0;
	/** The gap between the border integration area for the vertical leds */
	private double mVerticalGap = 0.0;

	/** The fraction of overlap from one to another led */
	private double mOverlapFraction = 0.0;
	
	/** Flag indicating that black borders are excluded in the image processing */
	private boolean mBlackBorderRemoval = true;
	
	/**
	 * Returns the horizontal depth (top and bottom) of the image integration as a fraction of the 
	 * image [0.0; 1.0]
	 * 
	 * @return The horizontal integration depth [0.0; 1.0] 
	 */
	public double getHorizontalDepth() {
		return mHorizontalDepth;
	}

	/**
	 * Sets the horizontal depth (top and bottom) of the image integration as a fraction of the 
	 * image [0.0; 1.0]
	 * 
	 * @param pHorizontalDepth The horizontal integration depth [0.0; 1.0] 
	 */
	public void setHorizontalDepth(double pHorizontalDepth) {
		if (mHorizontalDepth != pHorizontalDepth) {
			mHorizontalDepth = pHorizontalDepth;
			setChanged();
		}
	}

	/**
	 * Returns the horizontal gap (top and bottom) of the image integration area from the side of the 
	 * screen [0.0; 1.0]
	 * 
	 * @return The horizontal gap [0.0; 1.0] 
	 */
	public double getHorizontalGap() {
		return mHorizontalGap;
	}

	/**
	 * Sets the horizontal gap (top and bottom) of the image integration area from the side as a fraction of the 
	 * screen [0.0; 1.0]
	 * 
	 * @param pHorizontalGap The horizontal integration area gap from the side [0.0; 1.0] 
	 */
	public void setHorizontalGap(double pHorizontalGap) {
		if (mHorizontalGap != pHorizontalGap) {
			mHorizontalGap = pHorizontalGap;
			setChanged();
		}
	}

	/**
	 * Returns the vertical depth (left and right) of the image integration as a fraction of the 
	 * image [0.0; 1.0]
	 * 
	 * @return The vertical integration depth [0.0; 1.0] 
	 */
	public double getVerticalDepth() {
		return mVerticalDepth;
	}

	/**
	 * Sets the vertical depth (left and right) of the image integration as a fraction of the 
	 * image [0.0; 1.0]
	 * 
	 * @param pVerticalDepth The vertical integration depth [0.0; 1.0] 
	 */
	public void setVerticalDepth(double pVerticalDepth) {
		if (mVerticalDepth != pVerticalDepth) {
			mVerticalDepth = pVerticalDepth;
			setChanged();
		}
	}

	/**
	 * Returns the vertical gap (left and right) of the image integration area from the side of the 
	 * screen [0.0; 1.0]
	 * 
	 * @return The vertical gap [0.0; 1.0] 
	 */
	public double getVerticalGap() {
		return mVerticalGap;
	}

	/**
	 * Sets the horizontal gap (top and bottom) of the image integration area from the side as a fraction of the 
	 * screen [0.0; 1.0]
	 * 
	 * @param pHorizontalGap The horizontal integration area gap from the side [0.0; 1.0] 
	 */
	public void setVerticalGap(double pVerticalGap) {
		if (mVerticalGap != pVerticalGap) {
			mVerticalGap = pVerticalGap;
			setChanged();
		}
	}


	/**
	 * Returns the fractional overlap of one integration tile with its neighbors
	 * 
	 * @return The fractional overlap of the integration tiles
	 */
	public double getOverlapFraction() {
		return mOverlapFraction;
	}

	/**
	 * Sets the fractional overlap of one integration tile with its neighbors
	 * 
	 * @param pOverlapFraction The fractional overlap of the integration tiles
	 */
	public void setOverlapFraction(double pOverlapFraction) {
		if (mOverlapFraction != pOverlapFraction) {
			mOverlapFraction = pOverlapFraction;
			setChanged();
		}
	}

	/**
	 * Returns the black border removal flag
	 * @return True if black border removal is enabled else false
	 */
	public boolean isBlackBorderRemoval() {
		return mBlackBorderRemoval;
	}

	/**
	 * Sets the black border removal flag
	 * @param pBlackBorderRemoval True if black border removal is enabled else false
	 */
	public void setBlackBorderRemoval(boolean pBlackBorderRemoval) {
		if (mBlackBorderRemoval != pBlackBorderRemoval) {
			mBlackBorderRemoval = pBlackBorderRemoval;
			setChanged();
		}
	}
}
