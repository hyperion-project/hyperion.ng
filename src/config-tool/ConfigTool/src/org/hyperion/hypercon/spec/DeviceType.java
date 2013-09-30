package org.hyperion.hypercon.spec;

/**
 * Enumeration of known device types
 */
public enum DeviceType {
	/** WS2801 Led String device with one continuous shift-register */
	ws2801,
	/** Test device for writing color values to file-output */
	test,
	/** No device, no output is generated */
	none;
}
