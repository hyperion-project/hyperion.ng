package org.hyperion.hypercon.gui;

import java.awt.BasicStroke;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.event.ActionEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseMotionListener;
import java.awt.font.LineMetrics;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;
import java.io.File;
import java.net.URL;
import java.util.Vector;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.ImageIcon;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JMenu;
import javax.swing.JPopupMenu;
import javax.swing.SwingWorker;

import org.hyperion.hypercon.spec.Led;

public class JHyperionTv extends Component {

	private JPopupMenu mPopupMenu;
	private final Action mLoadAction = new AbstractAction("Load image...") {
		JFileChooser mImageChooser;
		@Override
		public void actionPerformed(ActionEvent e) {
			if (mImageChooser == null) {
				mImageChooser = new JFileChooser();
			}
			
			if (mImageChooser.showOpenDialog(JHyperionTv.this) != JFileChooser.APPROVE_OPTION) {
				return;
			}
			File file = mImageChooser.getSelectedFile();
			
			try {
				ImageIcon imageIcon = new ImageIcon(file.getAbsolutePath());
				Image image = imageIcon.getImage();
				mRawImage = image;
				repaint();
			} catch (Exception ex) {
				
			}
		}
	};
	
	private class SelectImageAction extends AbstractAction {
		private final String mImageName;
		SelectImageAction(String pImageName) {
			super(pImageName);
			mImageName = pImageName;
			
			ImageIcon image = loadImage();
			if (image != null) {
				Image scaledImage = image.getImage().getScaledInstance(32, 18, Image.SCALE_SMOOTH);
				ImageIcon scaledIcon = new ImageIcon(scaledImage, mImageName);
				putValue(SMALL_ICON, scaledIcon);
			}
		}
		@Override
		public void actionPerformed(ActionEvent e) {
			ImageIcon imageIcon = loadImage();
			if (imageIcon != null) {
				mRawImage = imageIcon.getImage();
				repaint();
			}
		}
		
		ImageIcon loadImage() {
			URL imageUrl = JHyperionTv.class.getResource(mImageName + ".png");
			if (imageUrl == null) {
				System.out.println("Failed to load image: " + mImageName);
				return null;
			}
			return new ImageIcon(imageUrl);
		}
	}
	
	private synchronized JPopupMenu getPopupMenu() {
		if (mPopupMenu == null) {
			mPopupMenu = new JPopupMenu();
			mPopupMenu.add(mLoadAction);
			
			JMenu selectMenu = new JMenu("Select Image");
			selectMenu.add(new SelectImageAction("TestImage_01"));
			selectMenu.add(new SelectImageAction("TestImage_02"));
			selectMenu.add(new SelectImageAction("TestImage_03"));
			selectMenu.add(new SelectImageAction("TestImage_04"));
			selectMenu.add(new SelectImageAction("TestImage_05"));
			selectMenu.add(new SelectImageAction("TestImageBBB_01"));
			selectMenu.add(new SelectImageAction("TestImageBBB_02"));
			selectMenu.add(new SelectImageAction("TestImageBBB_03"));
			mPopupMenu.add(selectMenu);
		}
		return mPopupMenu;
	}
	
	private Image mRawImage = new ImageIcon(JHyperionTv.class.getResource("TestImage_01.png")).getImage();
	
	int lightBorder = 100;
	int tvBorder    = 12;

	private class LedWrapper {
		public final Led led;
		
		public int lastX = 0;
		public int lastY = 0;
		
		public LedWrapper(Led pLed) {
			led = pLed;
		}
	}
	private final Vector<LedWrapper> mLeds2 = new Vector<>();
	
	public void setLeds(Vector<Led> pLeds) {
		mLeds2.clear();
		
		for (Led led : pLeds) {
			mLeds2.add(new LedWrapper(led));
		}
	}
	
	LedWrapper mSelLed = null;
	
	public JHyperionTv() {
		
		// Pre-cache the popup menu
		new SwingWorker<Object, Object>() {
			@Override
			protected Object doInBackground() throws Exception {
				return getPopupMenu();
			}
		}.execute();
		
		addMouseMotionListener(new MouseMotionListener() {
			@Override
			public void mouseMoved(MouseEvent e) {
				mSelLed = null;

				double x = (double)(e.getX() - lightBorder - tvBorder) / (getWidth() - lightBorder*2-tvBorder*2);
				double y = (double)(e.getY() - lightBorder - tvBorder) / (getHeight() - lightBorder*2-tvBorder*2);
				
				for (LedWrapper led : mLeds2) {
					if (led.led.mImageRectangle.contains(x, y) || (Math.abs(led.led.mLocation.getX() - x) < 0.01 && Math.abs(led.led.mLocation.getY() - y) < 0.01)) {
						mSelLed = led;
						break;
					}
				}
				
				repaint();
			}
			@Override
			public void mouseDragged(MouseEvent e) {
				
			}
		});
		addMouseListener(new MouseAdapter() {
			@Override
			public void mouseReleased(MouseEvent e) {
				showPopup(e);
			}
			@Override
			public void mousePressed(MouseEvent e) {
				showPopup(e);
			}
			private void showPopup(MouseEvent e) {
				if (!e.isPopupTrigger()) {
					return;
				}
				getPopupMenu().show(JHyperionTv.this, e.getX(), e.getY());
			}
		});
	}
	
	@Override
	public void paint(Graphics g) {
		super.paint(g);
		if (getWidth() <= 2*lightBorder+2*tvBorder+10 || getHeight() <= 2*lightBorder+2*tvBorder+10) {
			return;
		}
		
		Graphics2D g2d = (Graphics2D) g.create();
		g2d.setColor(Color.BLACK);
		g2d.fillRect(0, 0, getWidth(), getHeight());
		
		int screenWidth  = getWidth()  - 2*lightBorder;
		int screenHeight = getHeight() - 2*lightBorder;
		int imageWidth   = screenWidth - 2*tvBorder;
		int imageHeight  = screenHeight- 2*tvBorder;
		
		g2d.translate((getWidth() - imageWidth)/2, (getHeight() - imageHeight)/2);
		
		BufferedImage image = new BufferedImage(imageWidth, imageHeight, BufferedImage.TYPE_INT_RGB);
		image.getGraphics().drawImage(mRawImage, 0, 0, image.getWidth(), image.getHeight(), null);
		
		if (mLeds2 != null) {
			paintAllLeds(g2d, screenWidth, screenHeight, image);
		}
		
		g2d.setColor(Color.DARK_GRAY.darker());
		g2d.fillRect(-tvBorder, -tvBorder, screenWidth, screenHeight);
		g2d.drawImage(image, 0, 0, imageWidth, imageHeight, this);
		
		paintLedNumbers(g2d);
		
		for (LedWrapper led : mLeds2) {
			g2d.setColor(Color.GRAY);
			
			int xmin = (int)(led.led.mImageRectangle.getMinX() * (imageWidth-1));
			int xmax = (int)(led.led.mImageRectangle.getMaxX()* (imageWidth-1));

			int ymin = (int)(led.led.mImageRectangle.getMinY() * (imageHeight-1));
			int ymax = (int)(led.led.mImageRectangle.getMaxY() * (imageHeight-1));
			
			g2d.drawRect(xmin, ymin, (xmax-xmin), (ymax-ymin));
		}
		if (mSelLed != null) {
			g2d.setStroke(new BasicStroke(3.0f));
			g2d.setColor(Color.WHITE);
			
			int xmin = (int)(mSelLed.led.mImageRectangle.getMinX() * (imageWidth-1));
			int xmax = (int)(mSelLed.led.mImageRectangle.getMaxX()* (imageWidth-1));

			int ymin = (int)(mSelLed.led.mImageRectangle.getMinY() * (imageHeight-1));
			int ymax = (int)(mSelLed.led.mImageRectangle.getMaxY() * (imageHeight-1));
			
			g2d.drawRect(xmin, ymin, (xmax-xmin), (ymax-ymin));
		}
		
		Graphics2D gCopy = (Graphics2D)g.create();
		gCopy.setXORMode(Color.WHITE);
		gCopy.setFont(gCopy.getFont().deriveFont(20.0f));
		String ledCntStr = "Led count: " + mLeds2.size();
		gCopy.drawString(ledCntStr, getWidth()-150.0f, getHeight()-10.0f);
	}
	
	class LedPaint {
		int xmin;
		int xmax;
		int ymin;
		int ymax;
		
		int color; 
		int seqNr;
		
		int screenX;
		int screenY;
		double angle_rad;
	}
	
	private void paintAllLeds(Graphics2D g2d, int screenWidth, int screenHeight, BufferedImage image) {
		Dimension screenDimension = new Dimension(getWidth()-2*lightBorder-2*tvBorder, getHeight()-2*lightBorder-2*tvBorder);

		int imageWidth = image.getWidth();
		int imageHeight = image.getHeight();

		Vector<LedPaint> ledPaints = new Vector<>();
		
		for (LedWrapper led : mLeds2) {
			LedPaint ledPaint = new LedPaint();
			ledPaint.xmin = (int)(led.led.mImageRectangle.getMinX() * (imageWidth-1));
			ledPaint.xmax = (int)(led.led.mImageRectangle.getMaxX()* (imageWidth-1));
			ledPaint.ymin = (int)(led.led.mImageRectangle.getMinY() * (imageHeight-1));
			ledPaint.ymax = (int)(led.led.mImageRectangle.getMaxY() * (imageHeight-1));
			
			int red = 0;
			int green = 0;
			int blue = 0;
			int count = 0;
			
			for (int y = ledPaint.ymin; y <= ledPaint.ymax; ++y) {
				for (int x = ledPaint.xmin; x <= ledPaint.xmax; ++x) {
					int color = image.getRGB(x, y);
					red += (color >> 16) & 0xFF;
					green += (color >> 8) & 0xFF;
					blue += color & 0xFF;
					++count;
				}
			}
			ledPaint.color = count > 0 ? new Color(red / count, green/count, blue/count).getRGB() : 0;
			
			ledPaints.add(ledPaint);
			
			led.lastX = (int) (screenDimension.width  * led.led.mLocation.getX());
			led.lastY = (int) (screenDimension.height * led.led.mLocation.getY());
			
			ledPaint.screenX = led.lastX;
			ledPaint.screenY = led.lastY;
			ledPaint.angle_rad = 0.5*Math.PI - led.led.mSide.getAngle_rad();
			
			ledPaint.seqNr = led.led.mLedSeqNr;
		}
		
		for (int i=2; i<=180; i+=4) {
			int arcSize = 24 + (int)((i/12.0)*(i/12.0));

			for(LedPaint led : ledPaints) {
				int argb = 0x05000000 | (0x00ffffff & led.color);
				g2d.setColor(new Color(argb , true));

				g2d.translate(led.screenX, led.screenY);
				g2d.rotate(led.angle_rad);
				g2d.fillArc(-arcSize, -arcSize, 2*arcSize, 2*arcSize, 90-(i/2), i);
				g2d.rotate(-led.angle_rad);
				g2d.translate(-led.screenX, -led.screenY);
			}
		}
	}
	
	private void paintLedNumbers(Graphics2D pG2d) {
		pG2d.setColor(Color.GRAY);
		
		FontMetrics fontMetrics = pG2d.getFontMetrics();
		for (LedWrapper led : mLeds2) {
			String seqNrStr = "" + led.led.mLedSeqNr;
			Rectangle2D rect = fontMetrics.getStringBounds(seqNrStr, pG2d);

			pG2d.drawString("" + led.led.mLedSeqNr, (int)(led.lastX-rect.getWidth()/2), (int)(led.lastY+rect.getHeight()/2-2));
		}
	}
	
	public static void main(String[] pArgs)  {
		JFrame frame = new JFrame();
		frame.setSize(640, 480);
		frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		
		frame.getContentPane().setLayout(new BorderLayout());
		frame.getContentPane().add(new JHyperionTv());
		
		frame.setVisible(true);
	}
}
