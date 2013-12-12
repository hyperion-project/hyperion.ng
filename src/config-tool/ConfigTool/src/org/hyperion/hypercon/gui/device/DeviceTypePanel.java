package org.hyperion.hypercon.gui.device;

import java.awt.Dimension;

import javax.swing.JPanel;

import org.hyperion.hypercon.spec.DeviceConfig;

public abstract class DeviceTypePanel extends JPanel {

	protected final Dimension firstColMinDim = new Dimension(80, 10);
	protected final Dimension maxDim = new Dimension(1024, 20);
	
	protected DeviceConfig mDeviceConfig = null;
	
	public DeviceTypePanel() {
		super();
	}
	
	public void setDeviceConfig(DeviceConfig pDeviceConfig) {
		mDeviceConfig = pDeviceConfig;
	}
	
}
