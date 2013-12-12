package org.hyperion.hypercon.gui.device;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.GroupLayout;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import org.hyperion.hypercon.spec.DeviceConfig;

/**
 * Panel for configuring Ws2801 specific settings
 */
public class SerialPanel extends DeviceTypePanel {

	public static final String[] KnownOutputs = { "/dev/ttyS0", "/dev/ttyUSB0", "/dev/ttyAMA0", "/dev/null"};

	private JLabel mOutputLabel;
	private JComboBox<String> mOutputCombo;
	
	private JLabel mBaudrateLabel;
	private JSpinner mBaudrateSpinner;
	

	public SerialPanel() {
		super();
		
		initialise();
	}

	@Override
	public void setDeviceConfig(DeviceConfig pDeviceConfig) {
		super.setDeviceConfig(pDeviceConfig);

		mOutputCombo.setSelectedItem(mDeviceConfig.mOutput);
		((SpinnerNumberModel)mBaudrateSpinner.getModel()).setValue(mDeviceConfig.mBaudrate);
	}
	
	private void initialise() {
		mOutputLabel = new JLabel("Output: ");
		mOutputLabel.setMinimumSize(firstColMinDim);
		add(mOutputLabel);
		
		mOutputCombo = new JComboBox<>(KnownOutputs);
		mOutputCombo.setMaximumSize(maxDim);
		mOutputCombo.setEditable(true);
		mOutputCombo.addActionListener(mActionListener);
		add(mOutputCombo);
		
		mBaudrateLabel = new JLabel("Baudrate: ");
		mBaudrateLabel.setMinimumSize(firstColMinDim);
		add(mBaudrateLabel);
		
		mBaudrateSpinner = new JSpinner(new SpinnerNumberModel(1, 1, 1000000, 128));
		mBaudrateSpinner .setMaximumSize(maxDim);
		mBaudrateSpinner.addChangeListener(mChangeListener);
		add(mBaudrateSpinner);
	

		GroupLayout layout = new GroupLayout(this);
		layout.setAutoCreateGaps(true);
		setLayout(layout);
		
		layout.setHorizontalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mOutputLabel)
						.addComponent(mBaudrateLabel))
				.addGroup(layout.createParallelGroup()
						.addComponent(mOutputCombo)
						.addComponent(mBaudrateSpinner))
				);
		layout.setVerticalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mOutputLabel)
						.addComponent(mOutputCombo))
				.addGroup(layout.createParallelGroup()
						.addComponent(mBaudrateLabel)
						.addComponent(mBaudrateSpinner))
				);		
	}
	
	private ActionListener mActionListener = new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
			if (e.getSource() == mOutputCombo) {
				mDeviceConfig.mOutput = (String)mOutputCombo.getSelectedItem();
			} else if (e.getSource() == mBaudrateSpinner) {
				mDeviceConfig.mBaudrate = (Integer)mBaudrateSpinner.getValue();
			}
		}
	};
	
	private ChangeListener mChangeListener = new ChangeListener() {
		@Override
		public void stateChanged(ChangeEvent e) {
			mDeviceConfig.mBaudrate = (Integer)mBaudrateSpinner.getValue();
		}
	};
}
