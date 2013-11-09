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
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mXbmcCheck)
						.addComponent(mAddressField)
						.addComponent(mTcpPortSpinner)
						.addComponent(mMenuCombo)
						.addComponent(mVideoCombo)
						.addComponent(mPictureCombo)
						.addComponent(mAudioCombo)
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

			toggleEnabled(mMiscConfig.mXbmcCheckerEnabled);
		}
	};
}
