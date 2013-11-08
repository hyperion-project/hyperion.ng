package org.hyperion.hypercon.gui;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.BorderFactory;
import javax.swing.GroupLayout;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import org.hyperion.hypercon.spec.ColorConfig;
import org.hyperion.hypercon.spec.ColorSmoothingType;

public class ColorSmoothingPanel extends JPanel {

	private final ColorConfig mColorConfig;

	private JCheckBox mEnabledCheck;
	private JLabel mTypeLabel;
	private JComboBox<ColorSmoothingType> mTypeCombo;
	private JLabel mTimeLabel;
	private JSpinner mTimeSpinner;
	private JLabel mUpdateFrequencyLabel;
	private JSpinner mUpdateFrequencySpinner;

	public ColorSmoothingPanel(final ColorConfig pColorConfig) {
		super();
		
		mColorConfig = pColorConfig;
		
		initialise();
	}
	
	private void initialise() {
		setBorder(BorderFactory.createTitledBorder("Smoothing"));
		
		mEnabledCheck = new JCheckBox("Enabled");
		mEnabledCheck.setSelected(mColorConfig.mSmoothingEnabled);
		mEnabledCheck.addActionListener(mActionListener);
		add(mEnabledCheck);
		
		mTypeLabel = new JLabel("Type: ");
		add(mTypeLabel);
		
		mTypeCombo = new JComboBox<>(ColorSmoothingType.values());
		mTypeCombo.setSelectedItem(mColorConfig.mSmoothingType);
		mTypeCombo.addActionListener(mActionListener);
		add(mTypeCombo);
		
		mTimeLabel = new JLabel("Time [ms]: ");
		add(mTimeLabel);
		
		mTimeSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mSmoothingTime_ms, 1, 600, 100));
		mTimeSpinner.addChangeListener(mChangeListener);
		add(mTimeSpinner);
		
		mUpdateFrequencyLabel = new JLabel("Update Freq. [Hz]: ");
		add(mUpdateFrequencyLabel);
		
		mUpdateFrequencySpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mSmoothingUpdateFrequency_Hz, 1, 100, 1));
		mUpdateFrequencySpinner.addChangeListener(mChangeListener);
		add(mUpdateFrequencySpinner);

		GroupLayout layout = new GroupLayout(this);
		layout.setAutoCreateGaps(true);
		setLayout(layout);
		
		layout.setHorizontalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mEnabledCheck)
						.addComponent(mTypeLabel)
						.addComponent(mTimeLabel)
						.addComponent(mUpdateFrequencyLabel)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mEnabledCheck)
						.addComponent(mTypeCombo)
						.addComponent(mTimeSpinner)
						.addComponent(mUpdateFrequencySpinner)
						));
		layout.setVerticalGroup(layout.createSequentialGroup()
				.addComponent(mEnabledCheck)
				.addGroup(layout.createParallelGroup()
						.addComponent(mTypeLabel)
						.addComponent(mTypeCombo)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mTimeLabel)
						.addComponent(mTimeSpinner)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mUpdateFrequencyLabel)
						.addComponent(mUpdateFrequencySpinner)
						));
		
		toggleEnabled(mColorConfig.mSmoothingEnabled);
	}
	
	private void toggleEnabled(boolean pEnabled) {
		mTypeLabel.setEnabled(pEnabled);
		mTypeCombo.setEnabled(pEnabled);
		mTimeLabel.setEnabled(pEnabled);
		mTimeSpinner.setEnabled(pEnabled);
		mUpdateFrequencyLabel.setEnabled(pEnabled);
		mUpdateFrequencySpinner.setEnabled(pEnabled);
	}
	
	private final ActionListener mActionListener = new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
			mColorConfig.mSmoothingEnabled = mEnabledCheck.isSelected();
			mColorConfig.mSmoothingType = (ColorSmoothingType)mTypeCombo.getSelectedItem();
			
			toggleEnabled(mColorConfig.mSmoothingEnabled);
		}
	};
	
	private final ChangeListener mChangeListener = new ChangeListener() {
		@Override
		public void stateChanged(ChangeEvent e) {
			mColorConfig.mSmoothingTime_ms = (Integer)mTimeSpinner.getValue();
			mColorConfig.mSmoothingUpdateFrequency_Hz = (Integer)mUpdateFrequencySpinner.getValue();
		}
	};
}
