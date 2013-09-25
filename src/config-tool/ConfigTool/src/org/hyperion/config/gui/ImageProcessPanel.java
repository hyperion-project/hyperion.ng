package org.hyperion.config.gui;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Observable;
import java.util.Observer;

import javax.swing.GroupLayout;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import org.hyperion.config.spec.ImageProcessConfig;

public class ImageProcessPanel extends JPanel implements Observer {
	
	private final ImageProcessConfig mProcessConfig;
	
	private JLabel mHorizontalDepthLabel;
	private JSpinner mHorizontalDepthSpinner;
	private JLabel mVerticalDepthLabel;
	private JSpinner mVerticalDepthSpinner;
	
	private JLabel mOverlapLabel;
	private JSpinner mOverlapSpinner;
	
	private JLabel mBlackborderDetectorLabel;
	private JComboBox<String> mBlackborderDetectorCombo;

	public ImageProcessPanel(ImageProcessConfig pProcessConfig) {
		super();
		
		mProcessConfig = pProcessConfig;
		
		initialise();
		
		update(mProcessConfig, null);
	}
	
	private void initialise() {
		mHorizontalDepthLabel = new JLabel("Horizontal depth [%]:");
		mHorizontalDepthLabel.setPreferredSize(new Dimension(100, 30));
		mHorizontalDepthLabel.setMaximumSize(new Dimension(150, 30));
		add(mHorizontalDepthLabel);
		
		mHorizontalDepthSpinner = new JSpinner(new SpinnerNumberModel(5.0, 1.0, 100.0, 1.0));
		mHorizontalDepthSpinner.setPreferredSize(new Dimension(150, 30));
		mHorizontalDepthSpinner.setMaximumSize(new Dimension(250, 30));
		mHorizontalDepthSpinner.addChangeListener(mChangeListener);
		add(mHorizontalDepthSpinner);

		mVerticalDepthLabel = new JLabel("Vertical depth [%]:");
		mVerticalDepthLabel.setPreferredSize(new Dimension(100, 30));
		mVerticalDepthLabel.setMaximumSize(new Dimension(150, 30));
		add(mVerticalDepthLabel);
		
		mVerticalDepthSpinner = new JSpinner(new SpinnerNumberModel(5.0, 1.0, 100.0, 1.0));
		mVerticalDepthSpinner.setPreferredSize(new Dimension(150, 30));
		mVerticalDepthSpinner.setMaximumSize(new Dimension(250, 30));
		mVerticalDepthSpinner.addChangeListener(mChangeListener);
		add(mVerticalDepthSpinner);

		mOverlapLabel = new JLabel("Overlap [%]:");
		mOverlapLabel.setPreferredSize(new Dimension(100, 30));
		mOverlapLabel.setMaximumSize(new Dimension(150, 30));
		add(mOverlapLabel);
		
		mOverlapSpinner = new JSpinner(new SpinnerNumberModel(0.0, -100.0, 100.0, 1.0));
		mOverlapSpinner.setPreferredSize(new Dimension(150, 30));
		mOverlapSpinner.setMaximumSize(new Dimension(250, 30));
		mOverlapSpinner.addChangeListener(mChangeListener);
		add(mOverlapSpinner);
		
		mBlackborderDetectorLabel = new JLabel("Blackborder Detector:");
		add(mBlackborderDetectorLabel);
		
		mBlackborderDetectorCombo = new JComboBox<>(new String[] {"On", "Off"});
		mBlackborderDetectorCombo.setSelectedItem("On");
		mBlackborderDetectorCombo.setToolTipText("Enables or disables the blackborder detection and removal");
		mBlackborderDetectorCombo.addActionListener(mActionListener);
		add(mBlackborderDetectorCombo);
	
		GroupLayout layout = new GroupLayout(this);
		layout.setAutoCreateGaps(true);
		setLayout(layout);
		
		layout.setHorizontalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mHorizontalDepthLabel)
						.addComponent(mVerticalDepthLabel)
						.addComponent(mOverlapLabel)
						.addComponent(mBlackborderDetectorLabel)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mHorizontalDepthSpinner)
						.addComponent(mVerticalDepthSpinner)
						.addComponent(mOverlapSpinner)
						.addComponent(mBlackborderDetectorCombo)
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
				.addGroup(layout.createParallelGroup()
						.addComponent(mOverlapLabel)
						.addComponent(mOverlapSpinner)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mBlackborderDetectorLabel)
						.addComponent(mBlackborderDetectorCombo)
						)
						);
	}
	
	@Override
	public void update(Observable pObs, Object pArg) {
		if (pObs == mProcessConfig && pArg != this) {
			mHorizontalDepthSpinner.setValue(mProcessConfig.horizontalDepth * 100.0);
			mVerticalDepthSpinner.setValue(mProcessConfig.verticalDepth * 100.0);
			mOverlapSpinner.setValue(mProcessConfig.overlapFraction * 100.0);
		}
	}
	private final ActionListener mActionListener = new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
			mProcessConfig.blackBorderRemoval = (mBlackborderDetectorCombo.getSelectedItem() == "On");
		}
	};
	private final ChangeListener mChangeListener = new ChangeListener() {
		@Override
		public void stateChanged(ChangeEvent e) {
			mProcessConfig.horizontalDepth = ((Double)mHorizontalDepthSpinner.getValue())/100.0;
			mProcessConfig.verticalDepth = ((Double)mVerticalDepthSpinner.getValue())/100.0;
			mProcessConfig.overlapFraction = ((Double)mOverlapSpinner.getValue())/100.0;
			
			mProcessConfig.setChanged();
			mProcessConfig.notifyObservers(this);
		}
	};
}
