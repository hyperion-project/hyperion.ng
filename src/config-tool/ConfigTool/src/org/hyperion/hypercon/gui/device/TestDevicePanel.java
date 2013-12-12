package org.hyperion.hypercon.gui.device;

import javax.swing.GroupLayout;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;

import org.hyperion.hypercon.spec.DeviceConfig;

public class TestDevicePanel extends DeviceTypePanel {

	private JLabel mFilenameLabel;
	private JTextField mFilenameField;
	
	public TestDevicePanel() {
		super();
		
		initialise();
	}
	
	@Override
	public void setDeviceConfig(DeviceConfig pDeviceConfig) {
		super.setDeviceConfig(pDeviceConfig);
		
		mFilenameField.setText(mDeviceConfig.mOutput);
	}
	
	private void initialise() {
		mFilenameLabel = new JLabel("Filename: ");
		mFilenameLabel.setMinimumSize(firstColMinDim);
		add(mFilenameLabel);
		
		mFilenameField = new JTextField();
		mFilenameField.setMaximumSize(maxDim);
		mFilenameField.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				mDeviceConfig.mOutput = mFilenameField.getText();
			}
			@Override
			public void insertUpdate(DocumentEvent e) {
				mDeviceConfig.mOutput = mFilenameField.getText();
			}
			@Override
			public void changedUpdate(DocumentEvent e) {
				mDeviceConfig.mOutput = mFilenameField.getText();
			}
		});
		add(mFilenameField);

		GroupLayout layout = new GroupLayout(this);
		setLayout(layout);
		
		layout.setHorizontalGroup(layout.createSequentialGroup()
				.addComponent(mFilenameLabel)
				.addComponent(mFilenameField));
		layout.setVerticalGroup(layout.createParallelGroup()
				.addComponent(mFilenameLabel)
				.addComponent(mFilenameField));
	}
	
}
