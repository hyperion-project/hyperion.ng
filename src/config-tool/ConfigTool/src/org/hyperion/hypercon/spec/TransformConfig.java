package org.hyperion.hypercon.spec;

import java.util.Locale;

public class TransformConfig {
	/** The identifier of this ColorTransform configuration */
	public String mId = "";
	
	/** The saturation gain (in HSV space) */
	public double mSaturationGain = 1.0;
	/** The value gain (in HSV space) */
	public double mValueGain = 1.5;
	
	/** The minimum required RED-value (in RGB space) */
	public double mRedThreshold  = 0.1;
	/** The gamma-curve correct for the RED-value (in RGB space) */
	public double mRedGamma      = 2.0;
	/** The black-level of the RED-value (in RGB space) */
	public double mRedBlacklevel = 0.0;
	/** The white-level of the RED-value (in RGB space) */
	public double mRedWhitelevel = 0.8;
	
	/** The minimum required GREEN-value (in RGB space) */
	public double mGreenThreshold  = 0.1;
	/** The gamma-curve correct for the GREEN-value (in RGB space) */
	public double mGreenGamma      = 2.0;
	/** The black-level of the GREEN-value (in RGB space) */
	public double mGreenBlacklevel = 0.0;
	/** The white-level of the GREEN-value (in RGB space) */
	public double mGreenWhitelevel = 1.0;
	
	/** The minimum required BLUE-value (in RGB space) */
	public double mBlueThreshold  = 0.1;
	/** The gamma-curve correct for the BLUE-value (in RGB space) */
	public double mBlueGamma      = 2.0;
	/** The black-level of the BLUE-value (in RGB space) */
	public double mBlueBlacklevel = 0.0;
	/** The white-level of the BLUE-value (in RGB space) */
	public double mBlueWhitelevel = 1.0;

	public String toJsonString() {
		StringBuffer strBuf = new StringBuffer();

		strBuf.append("\t\t\t{\n");
		strBuf.append(hsvToJsonString() + ",\n");
		strBuf.append(rgbToJsonString() + "\n");
		strBuf.append("\t\t\t}");
		
		return strBuf.toString();
	}
	/**
	 * Creates the JSON string of the HSV-subconfiguration as used in the Hyperion deamon configfile
	 * 
	 * @return The JSON string of the HSV-config
	 */
	private String hsvToJsonString() {
		StringBuffer strBuf = new StringBuffer();
		strBuf.append("\t\t\t\t\"hsv\" :\n");
		strBuf.append("\t\t\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"saturationGain\" : %.4f,\n", mSaturationGain));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"valueGain\"      : %.4f\n", mValueGain));
		
		strBuf.append("\t\t\t\t}");
		return strBuf.toString();
	}
	
	/**
	 * Creates the JSON string of the RGB-subconfiguration as used in the Hyperion deamon configfile
	 * 
	 * @return The JSON string of the RGB-config
	 */
	private String rgbToJsonString() {
		StringBuffer strBuf = new StringBuffer();
		
		strBuf.append("\t\t\t\t\"red\" :\n");
		strBuf.append("\t\t\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"threshold\"  : %.4f,\n", mRedThreshold));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"gamma\"      : %.4f,\n", mRedGamma));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"blacklevel\" : %.4f,\n", mRedBlacklevel));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"whitelevel\" : %.4f\n",  mRedWhitelevel));
		strBuf.append("\t\t\t\t},\n");

		strBuf.append("\t\t\t\t\"green\" :\n");
		strBuf.append("\t\t\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"threshold\"  : %.4f,\n", mGreenThreshold));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"gamma\"      : %.4f,\n", mGreenGamma));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"blacklevel\" : %.4f,\n", mGreenBlacklevel));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"whitelevel\" : %.4f\n",  mGreenWhitelevel));
		strBuf.append("\t\t\t\t},\n");

		strBuf.append("\t\t\t\t\"blue\" :\n");
		strBuf.append("\t\t\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"threshold\"  : %.4f,\n", mBlueThreshold));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"gamma\"      : %.4f,\n", mBlueGamma));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"blacklevel\" : %.4f,\n", mBlueBlacklevel));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\t\t\"whitelevel\" : %.4f\n",  mBlueWhitelevel));
		strBuf.append("\t\t\t\t}");
		
		return strBuf.toString();
	}
}
