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
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JPanel;

import org.hyperion.config.LedFrameFactory;
import org.hyperion.config.LedString;

public class ConfigPanel extends JPanel {

	private final LedString ledString = new LedString();
	
	private final Action mSaveConfigAction = new AbstractAction("Create Hyperion Configuration") {
		@Override
		public void actionPerformed(ActionEvent e) {
			JFileChooser fileChooser = new JFileChooser();
			if (fileChooser.showSaveDialog(ConfigPanel.this) != JFileChooser.APPROVE_OPTION) {
				return;
			}

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
	
	private JPanel mSpecificationPanel;
	
	private MiscConfigPanel mMiscPanel;
	
	private JButton mSaveConfigButton;
	
	public ConfigPanel() {
		super();
		
		initialise();
		
		ledString.leds = LedFrameFactory.construct(ledString.mLedFrameConfig, ledString.mProcessConfig);
		mHyperionTv.setLeds(ledString.leds);
		
		final Observer observer = new Observer() {
			@Override
			public void update(Observable o, Object arg) {
				ledString.leds = LedFrameFactory.construct(ledString.mLedFrameConfig, ledString.mProcessConfig);
				mHyperionTv.setLeds(ledString.leds);
				mHyperionTv.repaint();
			}
		};
		ledString.mLedFrameConfig.addObserver(observer);
		ledString.mProcessConfig.addObserver(observer);
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
			
			mConstructionPanel = new LedFramePanel(ledString.mLedFrameConfig);
			mConstructionPanel.setBorder(BorderFactory.createTitledBorder("Construction"));
			mSpecificationPanel.add(mConstructionPanel);

			mIntegrationPanel = new ImageProcessPanel(ledString.mProcessConfig);
			mIntegrationPanel.setBorder(BorderFactory.createTitledBorder("Image Process"));
			mSpecificationPanel.add(mIntegrationPanel);
			
			mMiscPanel = new MiscConfigPanel(ledString.mMiscConfig);
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
}
