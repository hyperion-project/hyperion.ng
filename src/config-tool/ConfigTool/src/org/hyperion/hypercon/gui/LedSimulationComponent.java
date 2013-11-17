package org.hyperion.hypercon.gui;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Image;
import java.awt.event.ActionEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.image.BufferedImage;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.File;
import java.net.URL;
import java.util.Vector;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.ImageIcon;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JMenu;
import javax.swing.JPanel;
import javax.swing.JPopupMenu;
import javax.swing.JProgressBar;
import javax.swing.SwingWorker;

import org.hyperion.hypercon.LedFrameFactory;
import org.hyperion.hypercon.spec.ImageProcessConfig;
import org.hyperion.hypercon.spec.Led;
import org.hyperion.hypercon.spec.LedFrameConstruction;


public class LedSimulationComponent extends JPanel {

	private BufferedImage mTvImage = new BufferedImage(640, 480, BufferedImage.TYPE_INT_ARGB);
	
	private void setImage(Image pImage) {
		mTvImage.createGraphics().drawImage(pImage, 0, 0, mTvImage.getWidth(), mTvImage.getHeight(), null);
	}
	{
		Image image = new ImageIcon(LedSimulationComponent.class.getResource("TestImage_01.png")).getImage();
		mTvImage.createGraphics().drawImage(image, 0, 0, mTvImage.getWidth(), mTvImage.getHeight(), null);
	}

	private JPanel mTopPanel;
	private ImageComponent mTopLeftImage;
	private ImageComponent mTopImage;
	private ImageComponent mTopRightImage;
	
	private ImageComponent mLeftImage;
	private ImageComponent mRightImage;
	
	private JPanel mBottomPanel;
	private ImageComponent mBottomLeftImage;
	private ImageComponent mBottomImage;
	private ImageComponent mBottomRightImage;
	private JProgressBar mProgressBar;
	
	LedTvComponent mTvComponent;
	
	public LedSimulationComponent(Vector<Led> pLeds) {
		super();
		
		initialise(pLeds);
		
		setLeds(pLeds);
	}

	void initialise(Vector<Led> pLeds) {
		setBackground(Color.BLACK);
		setLayout(new BorderLayout());
		
		add(getTopPanel(), BorderLayout.NORTH);
		
		mLeftImage = new ImageComponent();
		mLeftImage.setPreferredSize(new Dimension(100,100));
		add(mLeftImage,  BorderLayout.WEST);
		mRightImage = new ImageComponent();
		mRightImage.setPreferredSize(new Dimension(100,100));
		add(mRightImage, BorderLayout.EAST);
		
		add(getBottomPanel(), BorderLayout.SOUTH);
		
		mTvComponent = new LedTvComponent(pLeds);
		mTvComponent.setImage(mTvImage);
		add(mTvComponent, BorderLayout.CENTER);
		
		mTvComponent.addMouseListener(mPopupListener);
	}
	
	private JPanel getTopPanel() {
		mTopPanel = new JPanel();
		mTopPanel.setPreferredSize(new Dimension(100,100));
		mTopPanel.setBackground(Color.BLACK);
		mTopPanel.setLayout(new BorderLayout());
		
		mTopLeftImage = new ImageComponent();
		mTopLeftImage.setPreferredSize(new Dimension(100,100));
		mTopPanel.add(mTopLeftImage,  BorderLayout.WEST);
		mTopImage = new ImageComponent();
		mTopPanel.add(mTopImage, BorderLayout.CENTER);
		mTopRightImage = new ImageComponent();
		mTopRightImage.setPreferredSize(new Dimension(100,100));
		mTopPanel.add(mTopRightImage, BorderLayout.EAST);
		
		return mTopPanel;
	}
	
	private JPanel getBottomPanel() {
		mBottomPanel = new JPanel();
		mBottomPanel.setPreferredSize(new Dimension(100,100));
		mBottomPanel.setBackground(Color.BLACK);
		mBottomPanel.setLayout(new BorderLayout());
		
		mBottomLeftImage = new ImageComponent();
		mBottomLeftImage.setPreferredSize(new Dimension(100,100));
		mBottomPanel.add(mBottomLeftImage,  BorderLayout.WEST);
		mBottomImage = new ImageComponent();
		mBottomPanel.add(mBottomImage,      BorderLayout.CENTER);
		mBottomRightImage = new ImageComponent();
		mBottomRightImage.setPreferredSize(new Dimension(100,100));
		mBottomPanel.add(mBottomRightImage, BorderLayout.EAST);
		
		mProgressBar = new JProgressBar(0, 100);
		mBottomPanel.add(mProgressBar, BorderLayout.SOUTH);
		
		return mBottomPanel;
	}
	
	
	LedSimulationWorker mWorker = null;
	
	public void setLeds(Vector<Led> pLeds) {
		mTvComponent.setLeds(pLeds);
		
		synchronized (LedSimulationComponent.this) {
			if (mWorker != null) {
				mWorker.cancel(true);
			}
			mWorker = null;
		}		
		mWorker = new LedSimulationWorker(mTvImage, pLeds);
		mProgressBar.setValue(0);
		mWorker.addPropertyChangeListener(new PropertyChangeListener() {
			@Override
			public void propertyChange(PropertyChangeEvent evt) {
				if (evt.getPropertyName() == "state") {
					if (evt.getNewValue() == SwingWorker.StateValue.STARTED) {
						mProgressBar.setVisible(true);
					} else if (evt.getNewValue() == SwingWorker.StateValue.DONE) {
						handleWorkerDone();
						mProgressBar.setVisible(false);
					}
				} else if (evt.getPropertyName() == "progress") {
					mProgressBar.setValue(mWorker.getProgress());
				}					
			}
			
			private void handleWorkerDone() {
				BufferedImage backgroundImage = null;
				synchronized(LedSimulationComponent.this) {
					if (mWorker == null) {
						return;
					}
					try {
						backgroundImage = mWorker.get();
						mWorker = null;
					} catch (Exception e) {}
				}
				if (backgroundImage == null) {
					return;
				}

				int width  = backgroundImage.getWidth();
				int height = backgroundImage.getHeight();
				int borderWidth  = (int) (backgroundImage.getWidth() * 0.1);
				int borderHeight = (int) (backgroundImage.getHeight() * 0.2);
				
				mTopLeftImage.setImage(backgroundImage.getSubimage(0, 0, borderWidth, borderHeight));
				mTopImage.setImage(backgroundImage.getSubimage(borderWidth, 0, width-2*borderWidth, borderHeight));
				mTopRightImage.setImage(backgroundImage.getSubimage(width-borderWidth, 0, borderWidth, borderHeight));

				mLeftImage.setImage(backgroundImage.getSubimage(0, borderHeight, borderWidth, height-2*borderHeight));
				mRightImage.setImage(backgroundImage.getSubimage(width-borderWidth, borderHeight, borderWidth, height-2*borderHeight));
				
				mBottomLeftImage.setImage(backgroundImage.getSubimage(0, height-borderHeight, borderWidth, borderHeight));
				mBottomImage.setImage(backgroundImage.getSubimage(borderWidth, height-borderHeight, width-2*borderWidth, borderHeight));
				mBottomRightImage.setImage(backgroundImage.getSubimage(width-borderWidth, height-borderHeight, borderWidth, borderHeight));
				
				mProgressBar.setValue(100);
				mProgressBar.setVisible(false);
				mWorker = null;
				
				LedSimulationComponent.this.repaint();
			}
		});
		mWorker.execute();
	}

	
	public static void main(String[] pArgs) {
		JFrame frame = new JFrame();
		frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		frame.setSize(800, 600);
		
		Vector<Led> leds = LedFrameFactory.construct(new LedFrameConstruction(), new ImageProcessConfig());
		
		LedSimulationComponent ledSimComp = new LedSimulationComponent(leds);
		
		frame.getContentPane().setLayout(new BorderLayout());
		frame.getContentPane().add(ledSimComp);
		
		frame.setVisible(true);
	}
	
	private final MouseListener mPopupListener = new MouseAdapter() {
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
			getPopupMenu().show(mTvComponent, e.getX(), e.getY());
		}
	};
	
	private JPopupMenu mPopupMenu;
	private final Action mLoadAction = new AbstractAction("Load image...") {
		JFileChooser mImageChooser;
		@Override
		public void actionPerformed(ActionEvent e) {
			if (mImageChooser == null) {
				mImageChooser = new JFileChooser();
			}
			
			if (mImageChooser.showOpenDialog(mTvComponent) != JFileChooser.APPROVE_OPTION) {
				return;
			}
			File file = mImageChooser.getSelectedFile();
			
			try {
				ImageIcon imageIcon = new ImageIcon(file.getAbsolutePath());
				Image image = imageIcon.getImage();
				
				mTvComponent.setImage(image);
//				setIma
			} catch (Exception ex) {
				
			}
		}
	};
	
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
				Image image = imageIcon.getImage();
				
				setImage(image);
				mTvComponent.setImage(image);
				
				repaint();
			}
		}
		
		ImageIcon loadImage() {
			URL imageUrl = LedSimulationComponent.class.getResource(mImageName + ".png");
			if (imageUrl == null) {
				System.out.println("Failed to load image: " + mImageName);
				return null;
			}
			return new ImageIcon(imageUrl);
		}
	}

}
