package org.hyperion.config.spec;

public class DeviceConfig {

	String mName     = "MyPi";
	DeviceType mType = DeviceType.ws2801;
	String mOutput   = "/dev/spidev0.0";
	int mBaudrate    = 48000;
	
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
