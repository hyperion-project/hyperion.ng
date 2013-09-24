package org.hyperion.config.gui;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.GroupLayout;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;

import org.hyperion.config.spec.BootSequence;

public class MiscConfigPanel extends JPanel {
	
	private JLabel mMenuLabel;
	private JComboBox<String> mMenuCombo;
	private JLabel mVideoLabel;
	private JComboBox<String> mVideoCombo;
	private JLabel mPictureLabel;
	private JComboBox<String> mPictureCombo;
	private JLabel mAudioLabel;
	private JComboBox<String> mAudioCombo;

	private JLabel mBlackborderDetectorLabel;
	private JComboBox<String> mBlackborderDetectorCombo;
	private JLabel mBootSequenceLabel;
	private JComboBox<BootSequence> mBootSequenceCombo;

	public MiscConfigPanel() {
		super();
		
		initialise();
	}
	
	private void initialise() {
		GroupLayout layout = new GroupLayout(this);
		layout.setAutoCreateGaps(true);
		setLayout(layout);
		
		mMenuLabel = new JLabel("XBMC Menu");
		add(mMenuLabel);
		
		mMenuCombo = new JComboBox<>(new String[] {"On", "Off"});
		mMenuCombo.setSelectedItem("Off");
		mMenuCombo.setToolTipText("Enables('On') or disbales('Off') the ambi-light in the XBMC Menu");
		mMenuCombo.addActionListener(mActionListener);
		add(mMenuCombo);

		mVideoLabel = new JLabel("Video");
		add(mVideoLabel);
		
		mVideoCombo = new JComboBox<>(new String[] {"On", "Off"});
		mVideoCombo.setSelectedItem("On");
		mVideoCombo.setToolTipText("Enables('On') or disbales('Off') the ambi-light during video playback");
		mVideoCombo.addActionListener(mActionListener);
		add(mVideoCombo);

		mPictureLabel = new JLabel("Picture");
		add(mPictureLabel);
		
		mPictureCombo = new JComboBox<>(new String[] {"On", "Off"});
		mPictureCombo.setSelectedItem("Off");
		mPictureCombo.setToolTipText("Enables('On') or disbales('Off') the ambi-light when viewing pictures");
		mPictureCombo.addActionListener(mActionListener);
		add(mPictureCombo);

		mAudioLabel = new JLabel("Audio");
		add(mAudioLabel);
		
		mAudioCombo = new JComboBox<>(new String[] {"On", "Off"});
		mAudioCombo.setSelectedItem("Off");
		mAudioCombo.setToolTipText("Enables('On') or disbales('Off') the ambi-light when listing to audio");
		mAudioCombo.addActionListener(mActionListener);
		add(mAudioCombo);
		
		mBlackborderDetectorLabel = new JLabel("Blackborder Detector:");
		add(mBlackborderDetectorLabel);
		
		mBlackborderDetectorCombo = new JComboBox<>(new String[] {"On", "Off"});
		mBlackborderDetectorCombo.setSelectedItem("On");
		mBlackborderDetectorCombo.setToolTipText("Enables or disables the blackborder detection and removal");
		add(mBlackborderDetectorCombo);
		
		mBootSequenceLabel = new JLabel("Boot Sequence:");
		add(mBootSequenceLabel);
		
		mBootSequenceCombo = new JComboBox<>(BootSequence.values());
		mBootSequenceCombo.setSelectedItem(BootSequence.rainbow);
		mBootSequenceCombo.setToolTipText("The sequence used on startup to verify proper working of all the leds");
		add(mBootSequenceCombo);
		
		layout.setHorizontalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mMenuLabel)
						.addComponent(mVideoLabel)
						.addComponent(mPictureLabel)
						.addComponent(mAudioLabel)
						.addComponent(mBlackborderDetectorLabel)
						.addComponent(mBootSequenceLabel)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mMenuCombo)
						.addComponent(mVideoCombo)
						.addComponent(mPictureCombo)
						.addComponent(mAudioCombo)
						.addComponent(mBlackborderDetectorCombo)
						.addComponent(mBootSequenceCombo)
						));
		layout.setVerticalGroup(layout.createSequentialGroup()
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
						.addComponent(mBlackborderDetectorLabel)
						.addComponent(mBlackborderDetectorCombo)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mBootSequenceLabel)
						.addComponent(mBootSequenceCombo)
						));
	}

	private final ActionListener mActionListener = new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
			
		}
	};
}
