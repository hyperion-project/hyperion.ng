package org.hyperion.hypercon.gui;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.beans.Transient;

import javax.swing.BorderFactory;
import javax.swing.GroupLayout;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import org.hyperion.hypercon.spec.ColorByteOrder;
import org.hyperion.hypercon.spec.DeviceConfig;
import org.hyperion.hypercon.spec.DeviceType;

public class DevicePanel extends JPanel {

	public static final String[] KnownOutputs = {"/dev/spidev0.0", "/dev/spidev0.1", "/dev/ttyS0", "/dev/ttyUSB0", "/dev/ttyprintk", "/home/pi/test.out", "/dev/null"};

	private final DeviceConfig mDeviceConfig;
	
	
	private JLabel mTypeLabel;
	private JComboBox<DeviceType> mTypeCombo;

	private JLabel mOutputLabel;
	private JComboBox<String> mOutputCombo;
	
	private JLabel mBaudrateLabel;
	private JSpinner mBaudrateSpinner;
	
	private JLabel mRgbLabel;
	private JComboBox<ColorByteOrder> mRgbCombo;
	
	public DevicePanel(DeviceConfig pDeviceConfig) {
		super();
		
		mDeviceConfig = pDeviceConfig;
		
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
		setBorder(BorderFactory.createTitledBorder("Device"));
		
		mOutputLabel = new JLabel("Output");
		add(mOutputLabel);
		
		mOutputCombo = new JComboBox<>(KnownOutputs);
		mOutputCombo.setEditable(true);
		mOutputCombo.setSelectedItem(mDeviceConfig.mOutput);
		mOutputCombo.addActionListener(mActionListener);
		add(mOutputCombo);
		
		mTypeLabel = new JLabel("LED Type:");
		add(mTypeLabel);
		
		mTypeCombo = new JComboBox<>(DeviceType.values());
		mTypeCombo.setSelectedItem(mDeviceConfig.mType);
		mTypeCombo.addActionListener(mActionListener);
		add(mTypeCombo);

		mBaudrateLabel = new JLabel("Baudrate");
		add(mBaudrateLabel);
		
		mBaudrateSpinner = new JSpinner(new SpinnerNumberModel(mDeviceConfig.mBaudrate, 1, 1000000, 128));
		mBaudrateSpinner.addChangeListener(mChangeListener);
		add(mBaudrateSpinner);
	
		mRgbLabel = new JLabel("RGB Byte Order");
		add(mRgbLabel);
		
		mRgbCombo = new JComboBox<>(ColorByteOrder.values());
		mRgbCombo.setSelectedItem(mDeviceConfig.mColorByteOrder);
		mRgbCombo.addActionListener(mActionListener);
		add(mRgbCombo);
		

		GroupLayout layout = new GroupLayout(this);
		layout.setAutoCreateGaps(true);
		setLayout(layout);
		
		layout.setHorizontalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mOutputLabel)
						.addComponent(mTypeLabel)
						.addComponent(mBaudrateLabel)
						.addComponent(mRgbLabel))
				.addGroup(layout.createParallelGroup()
						.addComponent(mOutputCombo)
						.addComponent(mTypeCombo)
						.addComponent(mBaudrateSpinner)
						.addComponent(mRgbCombo))
				);
		layout.setVerticalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mOutputLabel)
						.addComponent(mOutputCombo))
				.addGroup(layout.createParallelGroup()
						.addComponent(mTypeLabel)
						.addComponent(mTypeCombo))
				.addGroup(layout.createParallelGroup()
						.addComponent(mBaudrateLabel)
						.addComponent(mBaudrateSpinner))
				.addGroup(layout.createParallelGroup()
						.addComponent(mRgbLabel)
						.addComponent(mRgbCombo)));
	}
	
	private final ActionListener mActionListener = new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
			mDeviceConfig.mType = (DeviceType)mTypeCombo.getSelectedItem();
			mDeviceConfig.mOutput = (String)mOutputCombo.getSelectedItem();
			mDeviceConfig.mBaudrate = (Integer)mBaudrateSpinner.getValue();
			mDeviceConfig.mColorByteOrder = (ColorByteOrder)mRgbCombo.getSelectedItem();
		}
	};
	
	private final ChangeListener mChangeListener = new ChangeListener() {
		@Override
		public void stateChanged(ChangeEvent e) {
			mDeviceConfig.mBaudrate = (Integer)mBaudrateSpinner.getValue();
		}
	};
}
