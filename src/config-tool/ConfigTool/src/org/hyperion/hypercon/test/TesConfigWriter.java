package org.hyperion.hypercon.test;

import org.hyperion.hypercon.ConfigurationFile;
import org.hyperion.hypercon.spec.ColorByteOrder;
import org.hyperion.hypercon.spec.ColorConfig;
import org.hyperion.hypercon.spec.DeviceConfig;
import org.hyperion.hypercon.spec.DeviceType;
import org.hyperion.hypercon.spec.ImageProcessConfig;
import org.hyperion.hypercon.spec.LedFrameConstruction;
import org.hyperion.hypercon.spec.MiscConfig;

public class TesConfigWriter {

	public static void main(String[] pArgs) {
		DeviceConfig deviceConfig = new DeviceConfig();
		LedFrameConstruction frameConfig = new LedFrameConstruction();
		ColorConfig colorConfig = new ColorConfig();
		ImageProcessConfig imageConfig = new ImageProcessConfig();
		MiscConfig miscConfig = new MiscConfig();
		
		deviceConfig.mBaudrate = 4800;
		deviceConfig.mColorByteOrder = ColorByteOrder.BGR;
		deviceConfig.mName = "DAG";
		deviceConfig.mOutput = "/dev/null";
		deviceConfig.mType = DeviceType.lpd6803;
		
		
		ConfigurationFile configFile = new ConfigurationFile();
		configFile.store(deviceConfig);
		configFile.store(frameConfig);
		configFile.store(colorConfig);
		configFile.store(imageConfig);
		configFile.store(miscConfig);
		configFile.save("./HyperCon.dat");
		
		ConfigurationFile configFile2 = new ConfigurationFile();
		configFile2.load("./HyperCon.dat");
		configFile2.restore(deviceConfig);
		configFile2.restore(frameConfig);
		configFile2.restore(colorConfig);
		configFile2.restore(imageConfig);
		configFile2.restore(miscConfig);

		System.out.println(configFile2);
	}
}
