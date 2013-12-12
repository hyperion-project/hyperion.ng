package org.hyperion.hypercon.spec;

import org.hyperion.hypercon.gui.device.DeviceTypePanel;
import org.hyperion.hypercon.gui.device.LightPackPanel;
import org.hyperion.hypercon.gui.device.SerialPanel;
import org.hyperion.hypercon.gui.device.TestDevicePanel;
import org.hyperion.hypercon.gui.device.Ws2801Panel;

/**
 * Enumeration of known device types
 */
public enum DeviceType {
	/** WS2801 Led String device with one continuous shift-register (1 byte per color-channel) */
	ws2801("WS2801"),
	/** LDP8806 Led String device with one continuous shift-register (1 + 7 bits per color channel)*/
	lpd8806("LPD8806"),
	/** LDP6803 Led String device with one continuous shift-register (5 bits per color channel)*/
	lpd6803("LPD6803"),
	/** SEDU LED device */
	sedu("SEDU"),
	/** Lightberry device */
	lightberry("Lightberry"),
	/** Adalight device */
	adalight("Adalight"),
	/** Lightpack USB led device */
	lightpack("Lightpack"),
	/** Paintpack USB led device */
	paintpack("Paintpack"),
	/** Test device for writing color values to file-output */
	test("Test"),
	/** No device, no output is generated */
	none("None");
	
	/** The 'pretty' name of the device type */
	private final String mName;
	
	/** The device specific configuration panel */
	private DeviceTypePanel mConfigPanel;
	
	/**
	 * Constructs the DeviceType
	 * 
	 * @param name The 'pretty' name of the device type
	 * @param pConfigPanel The panel for device type specific configuration
	 */
	private DeviceType(final String name) {
		mName        = name;
	}
	
	/**
	 * Returns the configuration panel for the this device-type (or null if no configuration is required)
	 * 
	 * @return The panel for configuring this device type
	 */
	public DeviceTypePanel getConfigPanel(DeviceConfig pDeviceConfig) {
		if (mConfigPanel == null) {
			switch (this) {
			case ws2801:
			case lightberry:
			case lpd6803:
			case lpd8806:
				mConfigPanel = new Ws2801Panel();
				break;
			case test:
				mConfigPanel = new TestDevicePanel();
				break;
			case adalight:
			case sedu:
				mConfigPanel = new SerialPanel();
				break;
			case lightpack:
				mConfigPanel = new LightPackPanel();
				break;
			case paintpack:
			case none:
				break;
			}
		}
		if (mConfigPanel != null) {
			mConfigPanel.setDeviceConfig(pDeviceConfig);
		}
		return mConfigPanel;
	}
	
	@Override
	public String toString() {
		return mName;
	}
}
