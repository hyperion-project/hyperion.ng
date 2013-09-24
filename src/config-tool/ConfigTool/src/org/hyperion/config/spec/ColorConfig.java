package org.hyperion.config.spec;

import java.util.Locale;

public class ColorConfig {

	double mSaturationGain = 1.0;
	double mValueGain = 1.0;
	
	double mRedThreshold  = 0.0;
	double mRedGamma      = 1.0;
	double mRedBlacklevel = 0.0;
	double mRedWhitelevel = 1.0;
	
	double mGreenThreshold  = 0.0;
	double mGreenGamma      = 1.0;
	double mGreenBlacklevel = 0.0;
	double mGreenWhitelevel = 1.0;
	
	double mBlueThreshold  = 0.0;
	double mBlueGamma      = 1.0;
	double mBlueBlacklevel = 0.0;
	double mBlueWhitelevel = 1.0;
	
	public String toJsonString() {
		StringBuffer strBuf = new StringBuffer();
		strBuf.append("\t\"color\" :\n");
		strBuf.append("\t{\n");
		strBuf.append(hsvToJsonString() + ",\n");
		strBuf.append(rgbToJsonString() + "\n");
		strBuf.append("\t}");
		
		return strBuf.toString();
	}
	
	public String hsvToJsonString() {
		StringBuffer strBuf = new StringBuffer();
		strBuf.append("\t\t\"hsv\" :\n");
		strBuf.append("\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\tsaturationGain : %.4f,\n", mSaturationGain));
		strBuf.append(String.format(Locale.ROOT, "\t\t\tsaturationGain : %.4f\n", mValueGain));
		
		strBuf.append("\t\t}");
		return strBuf.toString();
	}
	
	public String rgbToJsonString() {
		StringBuffer strBuf = new StringBuffer();
		
		strBuf.append("\t\t\"red\" :\n");
		strBuf.append("\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\tthreshold  : %.4f,\n", mRedThreshold));
		strBuf.append(String.format(Locale.ROOT, "\t\t\tgamma      : %.4f,\n", mRedGamma));
		strBuf.append(String.format(Locale.ROOT, "\t\t\tblacklevel : %.4f,\n", mRedBlacklevel));
		strBuf.append(String.format(Locale.ROOT, "\t\t\twhitelevel : %.4f\n",  mRedWhitelevel));
		strBuf.append("\t\t},\n");

		strBuf.append("\t\t\"green\" :\n");
		strBuf.append("\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\tthreshold  : %.4f,\n", mGreenThreshold));
		strBuf.append(String.format(Locale.ROOT, "\t\t\tgamma      : %.4f,\n", mGreenGamma));
		strBuf.append(String.format(Locale.ROOT, "\t\t\tblacklevel : %.4f,\n", mGreenBlacklevel));
		strBuf.append(String.format(Locale.ROOT, "\t\t\twhitelevel : %.4f\n",  mGreenWhitelevel));
		strBuf.append("\t\t},\n");

		strBuf.append("\t\t\"blue\" :\n");
		strBuf.append("\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\tthreshold  : %.4f,\n", mBlueThreshold));
		strBuf.append(String.format(Locale.ROOT, "\t\t\tgamma      : %.4f,\n", mBlueGamma));
		strBuf.append(String.format(Locale.ROOT, "\t\t\tblacklevel : %.4f,\n", mBlueBlacklevel));
		strBuf.append(String.format(Locale.ROOT, "\t\t\twhitelevel : %.4f\n",  mBlueWhitelevel));
		strBuf.append("\t\t}");
		
		return strBuf.toString();
	}

}
