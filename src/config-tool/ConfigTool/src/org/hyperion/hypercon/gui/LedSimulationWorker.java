package org.hyperion.hypercon.gui;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.geom.Point2D;
import java.awt.image.BufferedImage;
import java.util.List;
import java.util.Vector;

import javax.swing.SwingWorker;

import org.hyperion.hypercon.spec.Led;

public class LedSimulationWorker extends SwingWorker<BufferedImage, Object> {

	private final BufferedImage mTvImage;
	
	private final Vector<Led> mLeds;
	
	public LedSimulationWorker(BufferedImage pTvImage, Vector<Led> pLeds) {
		super();
		
		mTvImage = pTvImage;
		mLeds    = pLeds;
	}
	
	class LedPaint {
		int color; 
		
		Point point;
		double angle_rad;
	}
	
	private final List<LedPaint> mLedPaints = new Vector<>();
	

	@Override
	protected BufferedImage doInBackground() throws Exception {
		Dimension imageDim = new Dimension(1280, 720);
		BufferedImage backgroundImage = new BufferedImage(imageDim.width, imageDim.height, BufferedImage.TYPE_INT_ARGB);
		
		mLedPaints.clear();
		
		setProgress(5);
		
		int imageWidth  = mTvImage.getWidth();
		int imageHeight = mTvImage.getHeight();
		for (Led led : mLeds) {
			LedPaint ledPaint = new LedPaint();
			
			// Determine the location and orientation of the led on the image
			ledPaint.point = tv2image(imageDim, led.mLocation);
			ledPaint.angle_rad = 0.5*Math.PI - led.mSide.getAngle_rad();
			
			// Determine the color of the led
			int xMin = (int)(led.mImageRectangle.getMinX() * (imageWidth-1));
			int xMax = (int)(led.mImageRectangle.getMaxX() * (imageWidth-1));
			int yMin = (int)(led.mImageRectangle.getMinY() * (imageHeight-1));
			int yMax = (int)(led.mImageRectangle.getMaxY() * (imageHeight-1));
			ledPaint.color = determineColor(xMin, xMax, yMin, yMax);
			
			mLedPaints.add(ledPaint);
		}
		
		setProgress(10);
		
		Graphics2D g2d = backgroundImage.createGraphics();
		// Clear the image with a black rectangle
		g2d.setColor(Color.BLACK);
		g2d.drawRect(0, 0, backgroundImage.getWidth(), backgroundImage.getHeight());
		paintAllLeds(g2d);

		return backgroundImage;
	}
	
	Point tv2image(Dimension pImageDim, Point2D point) {
		double tvWidthFraction  = (1.0 - 2*0.1);
		double tvHeightFraction = (1.0 - 2*0.2);
		
		double tvWidth = tvWidthFraction * pImageDim.width;
		double tvXIndex = point.getX()*tvWidth;
		double imageXIndex = tvXIndex + 0.1*pImageDim.width;
		
		double tvHeight = tvHeightFraction * pImageDim.height;
		double tvYIndex = point.getY()*tvHeight;
		double imageYIndex = tvYIndex + 0.2*pImageDim.height;

		return new Point((int)imageXIndex, (int)imageYIndex);
	}
	
	private int determineColor(int xMin, int xMax, int yMin, int yMax) {
		int red = 0;
		int green = 0;
		int blue = 0;
		int count = 0;
		
		for (int y = yMin; y <= yMax; ++y) {
			for (int x = xMin; x <= xMax; ++x) {
				int color = mTvImage.getRGB(x, y);
				red += (color >> 16) & 0xFF;
				green += (color >> 8) & 0xFF;
				blue += color & 0xFF;
				++count;
			}
		}
		
		return count > 0 ? new Color(red / count, green/count, blue/count).getRGB() : 0;
	}
	
	private void paintAllLeds(Graphics2D g2d) {

		for (int i=2; i<=180; i+=4) {
			if (isCancelled()) {
				return;
			}
			int arcSize = 24 + (int)((i/12.0)*(i/12.0));

			for(LedPaint led : mLedPaints) {
				int argb = 0x05000000 | (0x00ffffff & led.color);
				g2d.setColor(new Color(argb , true));

				g2d.translate(led.point.getX(), led.point.getY());
				g2d.rotate(led.angle_rad);
				g2d.fillArc(-arcSize, -arcSize, 2*arcSize, 2*arcSize, 90-(i/2), i);
				g2d.rotate(-led.angle_rad);
				g2d.translate(-led.point.getX(), -led.point.getY());

				setProgress(10+i/3);
			}
		}
	}
}
