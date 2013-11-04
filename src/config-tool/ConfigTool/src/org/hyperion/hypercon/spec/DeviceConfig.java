package org.hyperion.hypercon.spec;

/**
 * The device specific configuration
 */
public class DeviceConfig {

	/** The name of the device */
	public String mName     = "MyPi";
	/** The type specification of the device */
	public DeviceType mType = DeviceType.ws2801;
	/** The device 'file' name */
	public String mOutput   = "/dev/spidev0.0";
	/** The baudrate of the device */
	public int mBaudrate    = 1000000;
	/** The order of the color bytes */
	public ColorByteOrder mColorByteOrder = ColorByteOrder.RGB;
	
	/**
	 * Creates the JSON string of the configuration as used in the Hyperion daemon configfile
	 * 
	 * @return The JSON string of this DeviceConfig
	 */
	public String toJsonString() {
		StringBuffer strBuf = new StringBuffer();
		
		strBuf.append("\t/// Device configuration contains the following fields: \n");
		strBuf.append("\t/// * 'name'       : The user friendly name of the device (only used for display purposes)\n");
		strBuf.append("\t/// * 'type'       : The type of the device or leds (known types for now are 'ws2801', 'ldp6803', 'test' and 'none')\n");
		strBuf.append("\t/// * 'output'     : The output specification depends on selected device\n");
		strBuf.append("\t///                  - 'ws2801' this is the device (eg '/dev/spidev0.0')\n");
		strBuf.append("\t///                  - 'test' this is the file used to write test output (eg '/home/pi/hyperion.out')\n");
		strBuf.append("\t/// * 'rate'       : The baudrate of the output to the device (only applicable for 'ws2801')\n");
		strBuf.append("\t/// * 'colorOrder' : The order of the color bytes ('rgb', 'rbg', 'bgr', etc.).\n");

		strBuf.append("\t\"device\" :\n");
		strBuf.append("\t{\n");
		
		strBuf.append("\t\t\"name\"       : \"").append(mName).append("\",\n");
		strBuf.append("\t\t\"type\"       : \"").append(mType.name()).append("\",\n");
		strBuf.append("\t\t\"output\"     : \"").append(mOutput).append("\",\n");
		strBuf.append("\t\t\"rate\"       : ").append(mBaudrate).append(",\n");
		strBuf.append("\t\t\"colorOrder\" : \"").append(mColorByteOrder.name().toLowerCase()).append("\"\n");
		
		strBuf.append("\t}");
		
		return strBuf.toString();
	}
}
