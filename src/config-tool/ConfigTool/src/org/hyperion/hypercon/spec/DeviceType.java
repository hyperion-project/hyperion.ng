package org.hyperion.hypercon.spec;

/**
 * Enumeration of known device types
 */
public enum DeviceType {
	/** WS2801 Led String device with one continuous shift-register (1 byte per color-channel) */
	ws2801("WS2801"),
	/** LDP6803 Led String device with one continuous shift-register (5 bits per color channel)*/
	lpd6803("LPD6803"),
	/** SEDU LED device */
	sedu("SEDU"),
	/** Test device for writing color values to file-output */
	test("Test"),
	/** No device, no output is generated */
	none("None");
	
	private final String mName;
	
	private DeviceType(String name) {
		mName = name;
	}
	
	@Override
	public String toString() {
		return mName;
	}
}
