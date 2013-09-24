package org.hyperion.config.gui;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.io.IOException;
import java.util.Observable;
import java.util.Observer;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.GroupLayout;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import org.hyperion.config.LedFrameFactory;
import org.hyperion.config.LedString;
import org.hyperion.config.spec.LedFrameConstruction;

public class ConfigPanel extends JPanel {

	private final LedFrameConstruction mLedFrameSpec = new LedFrameConstruction();
	
	private final Action mSaveConfigAction = new AbstractAction("Create Hyperion Configuration") {
		@Override
		public void actionPerformed(ActionEvent e) {
			JFileChooser fileChooser = new JFileChooser();
			fileChooser.showSaveDialog(ConfigPanel.this);
			
			LedString ledString = new LedString();
			ledString.leds = LedFrameFactory.construct(mLedFrameSpec);
			
			try {
				ledString.saveConfigFile(fileChooser.getSelectedFile().getAbsolutePath());
			} catch (IOException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
		}
	};
	
	private JPanel mTvPanel;
	private JHyperionTv mHyperionTv;
	
	private JPanel mConstructionPanel;
	
	private JPanel mIntegrationPanel;
	
	private JLabel mHorizontalDepthLabel;
	private JSpinner mHorizontalDepthSpinner;
	private JLabel mVerticalDepthLabel;
	private JSpinner mVerticalDepthSpinner;
	
	private JPanel mSpecificationPanel;
	
	private MiscConfigPanel mMiscPanel;
	
	private JButton mSaveConfigButton;
	
	public ConfigPanel() {
		super();
		
		mLedFrameSpec.clockwiseDirection = true;
		
		mLedFrameSpec.topLeftCorner = true;
		mLedFrameSpec.topRightCorner= true;
		mLedFrameSpec.bottomLeftCorner= true;
		mLedFrameSpec.bottomRightCorner= true;
		
		mLedFrameSpec.topLedCnt    = 16;
		mLedFrameSpec.bottomLedCnt = 16;
		mLedFrameSpec.leftLedCnt   = 7;
		mLedFrameSpec.rightLedCnt  = 7;
		
		mLedFrameSpec.firstLedOffset = 0;
		
		initialise();
		
		mHyperionTv.setLeds(LedFrameFactory.construct(mLedFrameSpec));
		mLedFrameSpec.addObserver(new Observer() {
			@Override
			public void update(Observable o, Object arg) {
				mHyperionTv.setLeds(LedFrameFactory.construct(mLedFrameSpec));
				mHyperionTv.repaint();
			}
		});
	}
	
	private void initialise() {
		setLayout(new BorderLayout());
		
		add(getTvPanel(), BorderLayout.CENTER);
		add(getSpecificationPanel(), BorderLayout.WEST);
		
	}
	
	private JPanel getTvPanel() {
		if (mTvPanel == null) {
			mTvPanel = new JPanel();
			mTvPanel.setLayout(new BorderLayout());
				
			mHyperionTv = new JHyperionTv();
			mTvPanel.add(mHyperionTv, BorderLayout.CENTER);
		}
		return mTvPanel;
	}
	
	private JPanel getSpecificationPanel() {
		if (mSpecificationPanel == null) {
			mSpecificationPanel = new JPanel();
			mSpecificationPanel.setPreferredSize(new Dimension(300, 200));
			mSpecificationPanel.setLayout(new BoxLayout(mSpecificationPanel, BoxLayout.Y_AXIS));
			
			mConstructionPanel = new LedFramePanel(mLedFrameSpec);
			mConstructionPanel.setBorder(BorderFactory.createTitledBorder("Construction"));
			mSpecificationPanel.add(mConstructionPanel);

			mSpecificationPanel.add(getIntegrationPanel());
			
			mMiscPanel = new MiscConfigPanel();
			mMiscPanel.setBorder(BorderFactory.createTitledBorder("Misc"));
			mSpecificationPanel.add(mMiscPanel);
			
			JPanel panel = new JPanel(new BorderLayout());
			panel.setBorder(BorderFactory.createEmptyBorder(5,5,5,5));
			mSaveConfigButton = new JButton(mSaveConfigAction);
			panel.add(mSaveConfigButton, BorderLayout.SOUTH);
			mSpecificationPanel.add(panel);
		}
		return mSpecificationPanel;
	}

	private JPanel getIntegrationPanel() {
		if (mIntegrationPanel == null) {
			mIntegrationPanel = new JPanel();
			mIntegrationPanel.setBorder(BorderFactory.createTitledBorder("Image Process"));
			
			mHorizontalDepthLabel = new JLabel("Horizontal depth:");
			mHorizontalDepthLabel.setPreferredSize(new Dimension(100, 30));
			mHorizontalDepthLabel.setMaximumSize(new Dimension(150, 30));
			mIntegrationPanel.add(mHorizontalDepthLabel);
			
			mHorizontalDepthSpinner = new JSpinner(new SpinnerNumberModel(0.05, 0.01, 1.0, 0.01));
			mHorizontalDepthSpinner.setPreferredSize(new Dimension(150, 30));
			mHorizontalDepthSpinner.setMaximumSize(new Dimension(250, 30));
			mHorizontalDepthSpinner.addChangeListener(mChangeListener);
			mIntegrationPanel.add(mHorizontalDepthSpinner);

			mVerticalDepthLabel = new JLabel("Vertical depth:");
			mVerticalDepthLabel.setPreferredSize(new Dimension(100, 30));
			mVerticalDepthLabel.setMaximumSize(new Dimension(150, 30));
			mIntegrationPanel.add(mVerticalDepthLabel);
			
			mVerticalDepthSpinner = new JSpinner(new SpinnerNumberModel(0.05, 0.01, 1.0, 0.01));
			mVerticalDepthSpinner.setPreferredSize(new Dimension(150, 30));
			mVerticalDepthSpinner.setMaximumSize(new Dimension(250, 30));
			mVerticalDepthSpinner.addChangeListener(mChangeListener);
			mIntegrationPanel.add(mVerticalDepthSpinner);
			
			GroupLayout layout = new GroupLayout(mIntegrationPanel);
			mIntegrationPanel.setLayout(layout);
			
			layout.setHorizontalGroup(layout.createSequentialGroup()
					.addGroup(layout.createParallelGroup()
							.addComponent(mHorizontalDepthLabel)
							.addComponent(mVerticalDepthLabel)
							)
					.addGroup(layout.createParallelGroup()
							.addComponent(mHorizontalDepthSpinner)
							.addComponent(mVerticalDepthSpinner)
							)
							);
			layout.setVerticalGroup(layout.createSequentialGroup()
					.addGroup(layout.createParallelGroup()
							.addComponent(mHorizontalDepthLabel)
							.addComponent(mHorizontalDepthSpinner)
							)
					.addGroup(layout.createParallelGroup()
							.addComponent(mVerticalDepthLabel)
							.addComponent(mVerticalDepthSpinner)
							)
							);
			
		}
		return mIntegrationPanel;
	}
	
	private final ChangeListener mChangeListener = new ChangeListener() {
		@Override
		public void stateChanged(ChangeEvent e) {
			mLedFrameSpec.horizontalDepth = (Double)mHorizontalDepthSpinner.getValue();
			mLedFrameSpec.verticalDepth   = (Double)mVerticalDepthSpinner.getValue();
			
			mLedFrameSpec.setChanged();
			mLedFrameSpec.notifyObservers();
		}
	};
}
