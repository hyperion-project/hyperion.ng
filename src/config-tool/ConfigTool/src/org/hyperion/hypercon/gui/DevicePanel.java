package org.hyperion.hypercon.gui;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;

import org.hyperion.hypercon.spec.DeviceConfig;
import org.hyperion.hypercon.spec.RgbByteOrder;

public class DevicePanel extends JPanel {
	
	private final DeviceConfig mDeviceConfig;
	
	private JLabel mOutputLabel;
	private JComboBox<String> mOutputCombo;
	
	private JLabel mBaudrateLabel;
	private JComboBox<Integer> mBaudrateCombo;
	
	private JLabel mRgbLabel;
	private JComboBox<RgbByteOrder> mRgbCombo;
	
	public DevicePanel(DeviceConfig pDeviceConfig) {
		super();
		
		mDeviceConfig = pDeviceConfig;
		
		initialise();
	}
	
	private void initialise() {
		mOutputLabel = new JLabel("Output");
		add(mOutputLabel);
		
		mOutputCombo = new JComboBox<>(new String[] {"/dev/spidev0.0", " /dev/ttyUSB0", "/home/pi/test-output.txt", "/dev/null" });
		mOutputCombo.setEditable(true);
		mOutputCombo.setSelectedItem(mDeviceConfig.mOutput);
		mOutputCombo.addActionListener(mActionListener);
		add(mOutputCombo);
		
		mBaudrateLabel = new JLabel("Baudrate");
		add(mBaudrateLabel);
		
		mBaudrateCombo = new JComboBox<>();
		mRgbCombo.setSelectedItem(mDeviceConfig.mBaudrate);
		mRgbCombo.addActionListener(mActionListener);
		add(mBaudrateCombo);
	
		mRgbLabel = new JLabel("RGB Byte Order");
		add(mRgbLabel);
		
		mRgbCombo = new JComboBox<>(RgbByteOrder.values());
		mRgbCombo.setSelectedItem(mDeviceConfig.mRgbByteOrder);
		mRgbCombo.addActionListener(mActionListener);
		add(mRgbCombo);
	}
	
	private final ActionListener mActionListener = new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
			mDeviceConfig.mOutput = (String)mOutputCombo.getSelectedItem();
			mDeviceConfig.mBaudrate = (Integer)mBaudrateCombo.getSelectedItem();
			mDeviceConfig.mRgbByteOrder = (RgbByteOrder)mRgbCombo.getSelectedItem();
		}
	};
}
