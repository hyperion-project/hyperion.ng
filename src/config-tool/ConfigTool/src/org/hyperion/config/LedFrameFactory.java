package org.hyperion.config;

import java.awt.geom.Point2D;
import java.awt.geom.Rectangle2D;
import java.util.Vector;

import org.hyperion.config.spec.BorderSide;
import org.hyperion.config.spec.Led;
import org.hyperion.config.spec.LedFrameConstruction;

public class LedFrameFactory {

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
	
	public static Vector<Led> construct(LedFrameConstruction frameSpec) {
		double overlap_frac = 0.50;
		
		Vector<Led> mLeds = new Vector<>();
		
		int totalLedCount = frameSpec.getLedCount();
		if (totalLedCount <= 0) {
			return mLeds;
		}
		
		int iLed = (totalLedCount - frameSpec.firstLedOffset)%totalLedCount;
		if (iLed < 0) {
			iLed += totalLedCount;
		}
		if (frameSpec.topLeftCorner) {
			mLeds.add(createLed(frameSpec, iLed, 0.0, 0.0, overlap_frac, BorderSide.top_left));
			iLed = increase(frameSpec, iLed);
		}
		if (frameSpec.topLedCnt > 0) {
			int ledCnt = frameSpec.topLedCnt;
			double ledSpacing = (double)1.0/(ledCnt);
			for (int iTop=0; iTop<ledCnt; ++iTop) {
				double led_x = ledSpacing/2.0 + iTop * ledSpacing;
				double led_y = 0;

				mLeds.add(createLed(frameSpec, iLed, led_x, led_y, overlap_frac, BorderSide.top));
				iLed = increase(frameSpec, iLed);
			}
		}
		if (frameSpec.topRightCorner) {
			mLeds.add(createLed(frameSpec, iLed, 1.0, 0.0, overlap_frac, BorderSide.top_right));
			iLed = increase(frameSpec, iLed);
		}
		if (frameSpec.rightLedCnt > 0) {
			int ledCnt = frameSpec.rightLedCnt;
			double ledSpacing = 1.0/ledCnt;
			for (int iRight=0; iRight<ledCnt; ++iRight) {
				double led_x = 1.0;
				double led_y = ledSpacing/2.0 + iRight * ledSpacing;

				mLeds.add(createLed(frameSpec, iLed, led_x, led_y, overlap_frac, BorderSide.right));
				iLed = increase(frameSpec, iLed);
			}
		}
		if (frameSpec.bottomRightCorner) {
			mLeds.add(createLed(frameSpec, iLed, 1.0, 1.0, overlap_frac, BorderSide.bottom_right));
			iLed = increase(frameSpec, iLed);
		}
		if (frameSpec.bottomLedCnt > 0) {
			int ledCnt = frameSpec.topLedCnt;
			
			double ledSpacing = (double)1.0/ledCnt;
			for (int iBottom=(ledCnt-1); iBottom>=0; --iBottom) {
				if (iBottom > (frameSpec.bottomLedCnt-1)/2 && iBottom < ledCnt - frameSpec.bottomLedCnt/2) {
					continue;
				}
				double led_x = ledSpacing/2.0 + iBottom * ledSpacing;
				double led_y = 1.0;

				mLeds.add(createLed(frameSpec, iLed, led_x, led_y, overlap_frac, BorderSide.bottom));
				iLed = increase(frameSpec, iLed);
			}
		}
		if (frameSpec.bottomLeftCorner) {
			mLeds.add(createLed(frameSpec, iLed, 0.0, 1.0, overlap_frac, BorderSide.bottom_left));
			iLed = increase(frameSpec, iLed);
		}
		if (frameSpec.leftLedCnt > 0) {
			int ledCnt = frameSpec.leftLedCnt;
			double ledSpacing = (double)1.0/ledCnt;
			for (int iRight=(ledCnt-1); iRight>=0; --iRight) {
				double led_x = 0.0;
				double led_y = ledSpacing/2.0 + iRight * ledSpacing;

				mLeds.add(createLed(frameSpec, iLed, led_x, led_y, overlap_frac, BorderSide.left));
				iLed = increase(frameSpec, iLed);
			}
		}

		return mLeds;
	}
	
	private static Led createLed(LedFrameConstruction frameSpec, int seqNr, double x_frac, double y_frac, double overlap_frac, BorderSide pBorderSide) {
		Led led = new Led();
		led.mLedSeqNr = seqNr;
		led.mLocation = new Point2D.Double(x_frac, y_frac);
		led.mSide = pBorderSide;
		
		double widthFrac  = (1.0/frameSpec.topLedCnt * (1.0 + overlap_frac))/2.0;
		double heightFrac = (1.0/frameSpec.leftLedCnt * (1.0 + overlap_frac))/2.0;
		
		switch (pBorderSide) {
		case top_left: {
			led.mImageRectangle = new Rectangle2D.Double(0.0, 0.0, frameSpec.verticalDepth, frameSpec.horizontalDepth);
			break;
		}
		case top_right: {
			led.mImageRectangle = new Rectangle2D.Double(1.0-frameSpec.verticalDepth, 0.0, frameSpec.verticalDepth, frameSpec.horizontalDepth);
			break;
		}
		case bottom_left: {
			led.mImageRectangle = new Rectangle2D.Double(0.0, 1.0-frameSpec.horizontalDepth, frameSpec.verticalDepth, frameSpec.horizontalDepth);
			break;
		}
		case bottom_right: {
			led.mImageRectangle = new Rectangle2D.Double(1.0-frameSpec.verticalDepth, 1.0-frameSpec.horizontalDepth, frameSpec.verticalDepth, frameSpec.horizontalDepth);
			break;
		}
		case top:{
			double intXmin_frac = Math.max(0.0, x_frac-widthFrac);
			double intXmax_frac = Math.min(x_frac+widthFrac, 1.0);
			led.mImageRectangle = new Rectangle2D.Double(intXmin_frac, 0.0, intXmax_frac-intXmin_frac, frameSpec.horizontalDepth);
			
			break;
		}
		case bottom:
 {
			double intXmin_frac = Math.max(0.0, x_frac-widthFrac);
			double intXmax_frac = Math.min(x_frac+widthFrac, 1.0);
			
			led.mImageRectangle = new Rectangle2D.Double(intXmin_frac, 1.0-frameSpec.horizontalDepth, intXmax_frac-intXmin_frac, frameSpec.horizontalDepth);
			break;
		}
		case left: {
			double intYmin_frac = Math.max(0.0, y_frac-heightFrac);
			double intYmax_frac = Math.min(y_frac+heightFrac, 1.0);
			led.mImageRectangle = new Rectangle2D.Double(0.0, intYmin_frac, frameSpec.verticalDepth, intYmax_frac-intYmin_frac);
			break;
		}
		case right:
			double intYmin_frac = Math.max(0.0, y_frac-heightFrac);
			double intYmax_frac = Math.min(y_frac+heightFrac, 1.0);
			led.mImageRectangle = new Rectangle2D.Double(1.0-frameSpec.verticalDepth, intYmin_frac, frameSpec.verticalDepth, intYmax_frac-intYmin_frac);
			break;
		}
		
		return led;
	}
	

}
