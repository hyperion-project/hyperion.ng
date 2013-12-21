package org.hyperion.hypercon.gui;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.beans.Transient;

import javax.swing.BorderFactory;
import javax.swing.GroupLayout;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.JTextField;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;

import org.hyperion.hypercon.spec.MiscConfig;

public class XbmcPanel extends JPanel {

	private final MiscConfig mMiscConfig;
	
	private JCheckBox mXbmcCheck;
	
	private JLabel mAddressLabel;
	private JTextField mAddressField;
	
	private JLabel mTcpPortLabel;
	private JSpinner mTcpPortSpinner;
	
	private JLabel mMenuLabel;
	private JComboBox<String> mMenuCombo;
	private JLabel mVideoLabel;
	private JComboBox<String> mVideoCombo;
	private JLabel mPictureLabel;
	private JComboBox<String> mPictureCombo;
	private JLabel mAudioLabel;
	private JComboBox<String> mAudioCombo;
	private JLabel mScreensaverLabel;
	private JComboBox<String> mScreensaverCombo;
	private JLabel mEnable3DLabel;
	private JComboBox<String> mEnable3DCombo;
	
	public XbmcPanel(final MiscConfig pMiscConfig) {
		super();
		
		mMiscConfig = pMiscConfig;
		
		initialise();
	}
	
	@Override
	@Transient
	public Dimension getMaximumSize() {
		Dimension maxSize = super.getMaximumSize();
		Dimension prefSize = super.getPreferredSize();
		return new Dimension(maxSize.width, prefSize.height);
	}

	private void initialise() {
		setBorder(BorderFactory.createTitledBorder("XBMC Checker"));
		
		mXbmcCheck = new JCheckBox("Enabled");
		mXbmcCheck.setSelected(mMiscConfig.mXbmcCheckerEnabled);
		mXbmcCheck.addActionListener(mActionListener);
		add(mXbmcCheck);
		
		mAddressLabel = new JLabel("Server address:");
		add(mAddressLabel);
		
		mAddressField = new JTextField(mMiscConfig.mXbmcAddress);
		mAddressField.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				mMiscConfig.mXbmcAddress = mAddressField.getText();
			}
			@Override
			public void insertUpdate(DocumentEvent e) {
				mMiscConfig.mXbmcAddress = mAddressField.getText();
			}
			@Override
			public void changedUpdate(DocumentEvent e) {
				mMiscConfig.mXbmcAddress = mAddressField.getText();
			}
		});
		add(mAddressField);
		
		mTcpPortLabel = new JLabel("TCP port:");
		add(mTcpPortLabel);
		
		mTcpPortSpinner = new JSpinner(new SpinnerNumberModel(mMiscConfig.mXbmcTcpPort, 1, 65535, 1));
		mTcpPortSpinner.addChangeListener(mChangeListener);
		add(mTcpPortSpinner);
		
		
		mMenuLabel = new JLabel("XBMC Menu");
		add(mMenuLabel);
		
		mMenuCombo = new JComboBox<>(new String[] {"On", "Off"});
		mMenuCombo.setSelectedItem(mMiscConfig.mMenuOn? "On": "Off");
		mMenuCombo.setToolTipText("Enables('On') or disables('Off') the ambi-light in the XBMC Menu");
		mMenuCombo.addActionListener(mActionListener);
		add(mMenuCombo);

		mVideoLabel = new JLabel("Video");
		add(mVideoLabel);

		mVideoCombo = new JComboBox<>(new String[] {"On", "Off"});
		mVideoCombo.setSelectedItem(mMiscConfig.mVideoOn? "On": "Off");
		mVideoCombo.setToolTipText("Enables('On') or disables('Off') the ambi-light during video playback");
		mVideoCombo.addActionListener(mActionListener);
		add(mVideoCombo);

		mPictureLabel = new JLabel("Picture");
		add(mPictureLabel);
		
		mPictureCombo = new JComboBox<>(new String[] {"On", "Off"});
		mPictureCombo.setSelectedItem(mMiscConfig.mPictureOn? "On": "Off");
		mPictureCombo.setToolTipText("Enables('On') or disables('Off') the ambi-light when viewing pictures");
		mPictureCombo.addActionListener(mActionListener);
		add(mPictureCombo);

		mAudioLabel = new JLabel("Audio");
		add(mAudioLabel);
		
		mAudioCombo = new JComboBox<>(new String[] {"On", "Off"});
		mAudioCombo.setSelectedItem(mMiscConfig.mAudioOn? "On": "Off");
		mAudioCombo.setToolTipText("Enables('On') or disables('Off') the ambi-light when listing to audio");
		mAudioCombo.addActionListener(mActionListener);
		add(mAudioCombo);
		
		mScreensaverLabel = new JLabel("Screensaver");
		add(mScreensaverLabel);
		
		mScreensaverCombo = new JComboBox<>(new String[] {"On", "Off"});
		mScreensaverCombo.setSelectedItem(mMiscConfig.mScreensaverOn? "On": "Off");
		mScreensaverCombo.setToolTipText("Enables('On') or disables('Off') the ambi-light when the XBMC screensaver is active");
		mScreensaverCombo.addActionListener(mActionListener);
		add(mScreensaverCombo);
		
		mEnable3DLabel = new JLabel("3D checking");
		add(mEnable3DLabel);
		
		mEnable3DCombo = new JComboBox<>(new String[] {"On", "Off"});
		mEnable3DCombo.setSelectedItem(mMiscConfig.m3DCheckingEnabled ? "On": "Off");
		mEnable3DCombo.setToolTipText("Enables('On') or disables('Off') switching to 3D mode when a 3D video file is started");
		mEnable3DCombo.addActionListener(mActionListener);
		add(mEnable3DCombo);
		
		GroupLayout layout = new GroupLayout(this);
		layout.setAutoCreateGaps(true);
		setLayout(layout);
		
		layout.setHorizontalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mXbmcCheck)
						.addComponent(mAddressLabel)
						.addComponent(mTcpPortLabel)
						.addComponent(mMenuLabel)
						.addComponent(mVideoLabel)
						.addComponent(mPictureLabel)
						.addComponent(mAudioLabel)
						.addComponent(mScreensaverLabel)
						.addComponent(mEnable3DLabel)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mXbmcCheck)
						.addComponent(mAddressField)
						.addComponent(mTcpPortSpinner)
						.addComponent(mMenuCombo)
						.addComponent(mVideoCombo)
						.addComponent(mPictureCombo)
						.addComponent(mAudioCombo)
						.addComponent(mScreensaverCombo)
						.addComponent(mEnable3DCombo)
						));
		layout.setVerticalGroup(layout.createSequentialGroup()
				.addComponent(mXbmcCheck)
				.addGroup(layout.createParallelGroup()
						.addComponent(mAddressLabel)
						.addComponent(mAddressField)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mTcpPortLabel)
						.addComponent(mTcpPortSpinner)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mMenuLabel)
						.addComponent(mMenuCombo)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mVideoLabel)
						.addComponent(mVideoCombo)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mPictureLabel)
						.addComponent(mPictureCombo)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mAudioLabel)
						.addComponent(mAudioCombo)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mScreensaverLabel)
						.addComponent(mScreensaverCombo)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mEnable3DLabel)
						.addComponent(mEnable3DCombo)
						));

		toggleEnabled(mMiscConfig.mXbmcCheckerEnabled);
	}
	
	private void toggleEnabled(boolean pEnabled) {
		mAddressLabel.setEnabled(pEnabled);
		mAddressField.setEnabled(pEnabled);
		
		mTcpPortSpinner.setEnabled(pEnabled);
		mTcpPortLabel.setEnabled(pEnabled);
		
		mMenuLabel.setEnabled(pEnabled);
		mMenuCombo.setEnabled(pEnabled);
		mVideoLabel.setEnabled(pEnabled);
		mVideoCombo.setEnabled(pEnabled);
		mPictureLabel.setEnabled(pEnabled);
		mPictureCombo.setEnabled(pEnabled);
		mAudioLabel.setEnabled(pEnabled);
		mAudioCombo.setEnabled(pEnabled);
		mScreensaverLabel.setEnabled(pEnabled);
		mScreensaverCombo.setEnabled(pEnabled);
		mEnable3DLabel.setEnabled(pEnabled);
		mEnable3DCombo.setEnabled(pEnabled);
	}
	
	private final ChangeListener mChangeListener = new ChangeListener() {
		@Override
		public void stateChanged(ChangeEvent e) {
			mMiscConfig.mXbmcTcpPort = (Integer)mTcpPortSpinner.getValue();
		}	
	};
	
	private final ActionListener mActionListener = new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
			mMiscConfig.mXbmcCheckerEnabled = mXbmcCheck.isSelected();

			mMiscConfig.mMenuOn = (mMenuCombo.getSelectedItem() == "On");
			mMiscConfig.mVideoOn = (mVideoCombo.getSelectedItem() == "On");
			mMiscConfig.mPictureOn = (mPictureCombo.getSelectedItem() == "On");
			mMiscConfig.mAudioOn = (mAudioCombo.getSelectedItem() == "On");
			mMiscConfig.mScreensaverOn = (mScreensaverCombo.getSelectedItem() == "On");
			mMiscConfig.m3DCheckingEnabled = (mEnable3DCombo.getSelectedItem() == "On");

			toggleEnabled(mMiscConfig.mXbmcCheckerEnabled);
		}
	};
}
