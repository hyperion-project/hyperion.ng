package org.hyperion.hypercon.spec;

import java.util.Locale;

/**
 * The color tuning parameters of the different color channels (both in RGB space as in HSV space)
 */
public class ColorConfig {

	/** The saturation gain (in HSV space) */
	double mSaturationGain = 1.0;
	/** The value gain (in HSV space) */
	double mValueGain = 1.0;
	
	/** The minimum required RED-value (in RGB space) */
	double mRedThreshold  = 0.0;
	/** The gamma-curve correct for the RED-value (in RGB space) */
	double mRedGamma      = 1.0;
	/** The black-level of the RED-value (in RGB space) */
	double mRedBlacklevel = 0.0;
	/** The white-level of the RED-value (in RGB space) */
	double mRedWhitelevel = 1.0;
	
	/** The minimum required GREEN-value (in RGB space) */
	double mGreenThreshold  = 0.0;
	/** The gamma-curve correct for the GREEN-value (in RGB space) */
	double mGreenGamma      = 1.0;
	/** The black-level of the GREEN-value (in RGB space) */
	double mGreenBlacklevel = 0.0;
	/** The white-level of the GREEN-value (in RGB space) */
	double mGreenWhitelevel = 1.0;
	
	/** The minimum required BLUE-value (in RGB space) */
	double mBlueThreshold  = 0.0;
	/** The gamma-curve correct for the BLUE-value (in RGB space) */
	double mBlueGamma      = 1.0;
	/** The black-level of the BLUE-value (in RGB space) */
	double mBlueBlacklevel = 0.0;
	/** The white-level of the BLUE-value (in RGB space) */
	double mBlueWhitelevel = 1.0;
	
	/**
	 * Creates the JSON string of the configuration as used in the Hyperion daemon configfile
	 * 
	 * @return The JSON string of this ColorConfig
	 */
	public String toJsonString() {
		StringBuffer strBuf = new StringBuffer();
		strBuf.append("\t\"color\" :\n");
		strBuf.append("\t{\n");
		strBuf.append(hsvToJsonString() + ",\n");
		strBuf.append(rgbToJsonString() + "\n");
		strBuf.append("\t}");
		
		return strBuf.toString();
	}
	
	/**
	 * Creates the JSON string of the HSV-subconfiguration as used in the Hyperion deaomn configfile
	 * 
	 * @return The JSON string of the HSV-config
	 */
	private String hsvToJsonString() {
		StringBuffer strBuf = new StringBuffer();
		strBuf.append("\t\t\"hsv\" :\n");
		strBuf.append("\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\tsaturationGain : %.4f,\n", mSaturationGain));
		strBuf.append(String.format(Locale.ROOT, "\t\t\tsaturationGain : %.4f\n", mValueGain));
		
		strBuf.append("\t\t}");
		return strBuf.toString();
	}
	
	/**
	 * Creates the JSON string of the RGB-subconfiguration as used in the Hyperion deaomn configfile
	 * 
	 * @return The JSON string of the RGB-config
	 */
	private String rgbToJsonString() {
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
