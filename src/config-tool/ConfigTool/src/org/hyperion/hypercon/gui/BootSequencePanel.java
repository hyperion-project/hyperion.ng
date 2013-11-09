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
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import org.hyperion.hypercon.spec.BootSequence;
import org.hyperion.hypercon.spec.MiscConfig;

public class BootSequencePanel extends JPanel {

	private final MiscConfig mMiscConfig;

	private JCheckBox mBootSequenceCheck;
	private JLabel mBootSequenceLabel;
	private JComboBox<BootSequence> mBootSequenceCombo;
	private JLabel mBootSequenceLengthLabel;
	private JSpinner mBootSequenceLengthSpinner;
	
	public BootSequencePanel(final MiscConfig pMiscconfig) {
		super();
		
		mMiscConfig = pMiscconfig;
		
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
		setBorder(BorderFactory.createTitledBorder("Boot Sequence"));
		
		mBootSequenceCheck = new JCheckBox("Enabled");
		mBootSequenceCheck.setSelected(mMiscConfig.mBootsequenceEnabled);
		mBootSequenceCheck.addActionListener(mActionListener);
		add(mBootSequenceCheck);
		
		mBootSequenceLabel = new JLabel("Type:");
		mBootSequenceLabel.setEnabled(mMiscConfig.mBootsequenceEnabled);
		add(mBootSequenceLabel);
		
		mBootSequenceCombo = new JComboBox<>(BootSequence.values());
		mBootSequenceCombo.setSelectedItem(mMiscConfig.mBootSequence);
		mBootSequenceCombo.setToolTipText("The sequence used on startup to verify proper working of all the leds");
		mBootSequenceCombo.addActionListener(mActionListener);
		add(mBootSequenceCombo);
		
		mBootSequenceLengthLabel = new JLabel("Length [ms]");
		add(mBootSequenceLengthLabel);
		
		mBootSequenceLengthSpinner = new JSpinner(new SpinnerNumberModel(mMiscConfig.mBootSequenceLength_ms, 500, 3600000, 1000));
		mBootSequenceLengthSpinner.addChangeListener(mChangeListener);
		add(mBootSequenceLengthSpinner);
		
		
		GroupLayout layout = new GroupLayout(this);
		layout.setAutoCreateGaps(true);
		setLayout(layout);
		
		layout.setHorizontalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mBootSequenceCheck)
						.addComponent(mBootSequenceLabel)
						.addComponent(mBootSequenceLengthLabel)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mBootSequenceCheck)
						.addComponent(mBootSequenceCombo)
						.addComponent(mBootSequenceLengthSpinner)
						));
		layout.setVerticalGroup(layout.createSequentialGroup()
				.addComponent(mBootSequenceCheck)
				.addGroup(layout.createParallelGroup()
						.addComponent(mBootSequenceLabel)
						.addComponent(mBootSequenceCombo)
						)
				.addGroup(layout.createParallelGroup()
						.addComponent(mBootSequenceLengthLabel)
						.addComponent(mBootSequenceLengthSpinner)
						));
		
		toggleEnabled(mMiscConfig.mBootsequenceEnabled);
	}
	
	private void toggleEnabled(boolean pEnabled) {
		mBootSequenceLabel.setEnabled(pEnabled);
		mBootSequenceCombo.setEnabled(pEnabled);
		mBootSequenceLengthLabel.setEnabled(pEnabled);
		mBootSequenceLengthSpinner.setEnabled(pEnabled);
	}
	
	private final ActionListener mActionListener = new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
			mMiscConfig.mBootsequenceEnabled = mBootSequenceCheck.isSelected();
			mMiscConfig.mBootSequence = (BootSequence) mBootSequenceCombo.getSelectedItem();
			
			toggleEnabled(mMiscConfig.mBootsequenceEnabled);
		}
	};
	
	private final ChangeListener mChangeListener = new ChangeListener() {
		@Override
		public void stateChanged(ChangeEvent e) {
			mMiscConfig.mBootSequenceLength_ms = (Integer)mBootSequenceLengthSpinner.getValue();
		}
	};
}
