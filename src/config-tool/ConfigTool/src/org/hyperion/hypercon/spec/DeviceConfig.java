package org.hyperion.hypercon.spec;

/**
 * The device specific configuration
 */
public class DeviceConfig {

	/** The name of the device */
	String mName     = "MyPi";
	/** The type specification of the device */
	DeviceType mType = DeviceType.ws2801;
	/** The device 'file' name */
	String mOutput   = "/dev/spidev0.0";
	/** The baudrate of the device */
	int mBaudrate    = 48000;
	
	/**
	 * Creates the JSON string of the configuration as used in the Hyperion daemon configfile
	 * 
	 * @return The JSON string of this DeviceConfig
	 */
	public String toJsonString() {
		StringBuffer strBuf = new StringBuffer();
		strBuf.append("\t\"device\" :\n");
		strBuf.append("\t{\n");
		
		strBuf.append("\t\t\"name\"   : \"").append(mName).append("\",\n");
		strBuf.append("\t\t\"type\"   : \"").append(mType).append("\",\n");
		strBuf.append("\t\t\"output\" : \"").append(mOutput).append("\",\n");
		strBuf.append("\t\t\"rate\"   : ").append(mBaudrate).append("\n");
		
		strBuf.append("\t}");
		
		return strBuf.toString();
	}

}
