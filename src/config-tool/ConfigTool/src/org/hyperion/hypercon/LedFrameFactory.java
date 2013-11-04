package org.hyperion.hypercon;

import java.awt.geom.Point2D;
import java.awt.geom.Rectangle2D;
import java.util.Collections;
import java.util.Comparator;
import java.util.Vector;

import org.hyperion.hypercon.spec.BorderSide;
import org.hyperion.hypercon.spec.ImageProcessConfig;
import org.hyperion.hypercon.spec.Led;
import org.hyperion.hypercon.spec.LedFrameConstruction;

/**
 * The LedFrameFactory translates user specifications (number of leds, etc) to actual led 
 * specifications (location of led, depth and width of integration, etc)  
 */
public class LedFrameFactory {

	/**
	 * Convenience method for increasing the led counter (it might actually decrease if the frame is 
	 * counter clockwise)
	 * 
	 * @param frameSpec The specification of the led-frame
	 * @param pLedCounter The current led counter
	 * @return The counter/index of the next led
	 */
	private static int increase(LedFrameConstruction frameSpec, int pLedCounter) {
		if (frameSpec.clockwiseDirection) {
			return (pLedCounter+1)%frameSpec.getLedCount();
		} else {
			if (pLedCounter == 0) {
				return frameSpec.getLedCount() - 1;
			}
			return pLedCounter -1;
		}
		
	}
	
	/**
	 * Translate a 'frame' and picture integration specification to per-led specification
	 * 
	 * @param frameSpec The specification of the led frame
	 * @param processConfig The picture integration specification
	 *  
	 * @return The per-led specification
	 */
	public static Vector<Led> construct(LedFrameConstruction frameSpec, ImageProcessConfig processConfig) {
		Vector<Led> mLeds = new Vector<>();
		
		int totalLedCount = frameSpec.getLedCount();
		if (totalLedCount <= 0) {
			return mLeds;
		}
		
		// Determine the led-number of the top-left led
		int iLed = (totalLedCount - frameSpec.firstLedOffset)%totalLedCount;
		if (iLed < 0) {
			iLed += totalLedCount;
		}
		
		// Construct the top-left led (if top-left is enabled)
		if (frameSpec.topCorners) {
			mLeds.add(createLed(frameSpec, processConfig, iLed, 0.0, 0.0, processConfig.getOverlapFraction(), BorderSide.top_left));
			iLed = increase(frameSpec, iLed);
		}
		
		// Construct all leds along the top of the screen (if any)
		if (frameSpec.topLedCnt > 0) {
			// Determine the led-spacing
			int ledCnt = frameSpec.topLedCnt;
			double ledSpacing = (double)1.0/(ledCnt);

			for (int iTop=0; iTop<ledCnt; ++iTop) {
				// Compute the location of this led
				double led_x = ledSpacing/2.0 + iTop * ledSpacing;
				double led_y = 0;

				// Construct and add the single led specification to the list of leds
				mLeds.add(createLed(frameSpec, processConfig, iLed, led_x, led_y, processConfig.getOverlapFraction(), BorderSide.top));
				iLed = increase(frameSpec, iLed);
			}
		}
		
		// Construct the top-right led (if top-right is enabled)
		if (frameSpec.topCorners) {
			mLeds.add(createLed(frameSpec, processConfig, iLed, 1.0, 0.0, processConfig.getOverlapFraction(), BorderSide.top_right));
			iLed = increase(frameSpec, iLed);
		}
		
		// Construct all leds along the right of the screen (if any)
		if (frameSpec.rightLedCnt > 0) {
			// Determine the led-spacing
			int ledCnt = frameSpec.rightLedCnt;
			double ledSpacing = 1.0/ledCnt;

			for (int iRight=0; iRight<ledCnt; ++iRight) {
				// Compute the location of this led
				double led_x = 1.0;
				double led_y = ledSpacing/2.0 + iRight * ledSpacing;

				// Construct and add the single led specification to the list of leds
				mLeds.add(createLed(frameSpec, processConfig, iLed, led_x, led_y, processConfig.getOverlapFraction(), BorderSide.right));
				iLed = increase(frameSpec, iLed);
			}
		}
		
		// Construct the bottom-right led (if bottom-right is enabled)
		if (frameSpec.bottomCorners) {
			mLeds.add(createLed(frameSpec, processConfig, iLed, 1.0, 1.0, processConfig.getOverlapFraction(), BorderSide.bottom_right));
			iLed = increase(frameSpec, iLed);
		}
		
		// Construct all leds along the bottom of the screen (if any)
		if (frameSpec.bottomLedCnt > 0) {
			// Determine the led-spacing (based on top-leds [=bottom leds + gap size])
			int ledCnt = frameSpec.topLedCnt;
			double ledSpacing = (double)1.0/ledCnt;

			for (int iBottom=(ledCnt-1); iBottom>=0; --iBottom) {
				// Special case for the bottom-gap
				if (iBottom > (frameSpec.bottomLedCnt-1)/2 && iBottom < ledCnt - frameSpec.bottomLedCnt/2) {
					continue;
				}
				// Compute the location of this led
				double led_x = ledSpacing/2.0 + iBottom * ledSpacing;
				double led_y = 1.0;

				// Construct and add the single led specification to the list of leds
				mLeds.add(createLed(frameSpec, processConfig, iLed, led_x, led_y, processConfig.getOverlapFraction(), BorderSide.bottom));
				iLed = increase(frameSpec, iLed);
			}
		}
		
		// Construct the bottom-left led (if bottom-left is enabled)
		if (frameSpec.bottomCorners) {
			mLeds.add(createLed(frameSpec, processConfig, iLed, 0.0, 1.0, processConfig.getOverlapFraction(), BorderSide.bottom_left));
			iLed = increase(frameSpec, iLed);
		}
		
		// Construct all leds along the left of the screen (if any)
		if (frameSpec.leftLedCnt > 0) {
			// Determine the led-spacing
			int ledCnt = frameSpec.leftLedCnt;
			double ledSpacing = (double)1.0/ledCnt;
			
			for (int iRight=(ledCnt-1); iRight>=0; --iRight) {
				// Compute the location of this led
				double led_x = 0.0;
				double led_y = ledSpacing/2.0 + iRight * ledSpacing;

				// Construct and add the single led specification to the list of leds
				mLeds.add(createLed(frameSpec, processConfig, iLed, led_x, led_y, processConfig.getOverlapFraction(), BorderSide.left));
				iLed = increase(frameSpec, iLed);
			}
		}

		Collections.sort(mLeds, new Comparator<Led>() {
			@Override
			public int compare(Led o1, Led o2) {
				return Integer.compare(o1.mLedSeqNr, o2.mLedSeqNr);
			}
		});
		return mLeds;
	}
	
	/**
	 * Constructs the specification of a single led
	 * 
	 * @param pFrameSpec The overall led-frame specification
	 * @param pProcessSpec The overall image-processing specification
	 * @param seqNr The number of the led
	 * @param x_frac The x location of the led in fractional range [0.0; 1.0]
	 * @param y_frac The y location of the led in fractional range [0.0; 1.0]
	 * @param overlap_frac The fractional overlap of the led integration with its neighbor
	 * @param pBorderSide The side on which the led is located
	 * 
	 * @return The image integration specifications of the single led
	 */
	private static Led createLed(LedFrameConstruction pFrameSpec, ImageProcessConfig pProcessSpec, int seqNr, double x_frac, double y_frac, double overlap_frac, BorderSide pBorderSide) {
		Led led = new Led();
		led.mLedSeqNr = seqNr;
		led.mLocation = new Point2D.Double(x_frac, y_frac);
		led.mSide = pBorderSide;
		
		double xFrac      = pProcessSpec.getVerticalGap() + (1.0-2*pProcessSpec.getVerticalGap()) * x_frac;	
		double yFrac      = pProcessSpec.getHorizontalGap() + (1.0-2*pProcessSpec.getHorizontalGap()) * y_frac;	
		double widthFrac  = ((1.0-2*pProcessSpec.getVerticalGap())/pFrameSpec.topLedCnt * (1.0 + overlap_frac))/2.0;
		double heightFrac = ((1.0-2*pProcessSpec.getHorizontalGap())/pFrameSpec.leftLedCnt * (1.0 + overlap_frac))/2.0;
		
		double horizontalDepth = Math.min(1.0 - pProcessSpec.getHorizontalGap(), pProcessSpec.getHorizontalDepth());
		double verticalDepth = Math.min(1.0 - pProcessSpec.getVerticalGap(), pProcessSpec.getVerticalDepth());
		
		switch (pBorderSide) {
		case top_left: {
			led.mImageRectangle = new Rectangle2D.Double(
					pProcessSpec.getVerticalGap(), 
					pProcessSpec.getHorizontalGap(),
					verticalDepth, 
					horizontalDepth);
			break;
		}
		case top_right: {
			led.mImageRectangle = new Rectangle2D.Double(
					1.0-pProcessSpec.getVerticalGap()-verticalDepth,
					pProcessSpec.getHorizontalGap(),
					verticalDepth, 
					horizontalDepth);
			break;
		}
		case bottom_left: {
			led.mImageRectangle = new Rectangle2D.Double(
					pProcessSpec.getVerticalGap(),
					1.0-pProcessSpec.getHorizontalGap()-horizontalDepth,
					verticalDepth,
					horizontalDepth);
			break;
		}
		case bottom_right: {
			led.mImageRectangle = new Rectangle2D.Double(
					1.0-pProcessSpec.getVerticalGap()-verticalDepth, 
					1.0-pProcessSpec.getHorizontalGap()-horizontalDepth,
					verticalDepth,
					horizontalDepth);
			break;
		}
		case top:{
			double intXmin_frac = Math.max(0.0, xFrac-widthFrac);
			double intXmax_frac = Math.min(xFrac+widthFrac, 1.0);
			led.mImageRectangle = new Rectangle2D.Double(
					intXmin_frac, 
					pProcessSpec.getHorizontalGap(), 
					intXmax_frac-intXmin_frac, 
					horizontalDepth);
			
			break;
		}
		case bottom:
		{
			double intXmin_frac = Math.max(0.0, xFrac-widthFrac);
			double intXmax_frac = Math.min(xFrac+widthFrac, 1.0);
			
			led.mImageRectangle = new Rectangle2D.Double(
					intXmin_frac, 
					1.0-pProcessSpec.getHorizontalGap()-horizontalDepth, 
					intXmax_frac-intXmin_frac, 
					horizontalDepth);
			break;
		}
		case left: {
			double intYmin_frac = Math.max(0.0, yFrac-heightFrac);
			double intYmax_frac = Math.min(yFrac+heightFrac, 1.0);
			led.mImageRectangle = new Rectangle2D.Double(
					pProcessSpec.getVerticalGap(), 
					intYmin_frac, 
					verticalDepth, 
					intYmax_frac-intYmin_frac);
			break;
		}
		case right:
			double intYmin_frac = Math.max(0.0, yFrac-heightFrac);
			double intYmax_frac = Math.min(yFrac+heightFrac, 1.0);
			led.mImageRectangle = new Rectangle2D.Double(
					1.0-pProcessSpec.getVerticalGap()-verticalDepth, 
					intYmin_frac, 
					verticalDepth, 
					intYmax_frac-intYmin_frac);
			break;
		}
		
		return led;
	}

}
