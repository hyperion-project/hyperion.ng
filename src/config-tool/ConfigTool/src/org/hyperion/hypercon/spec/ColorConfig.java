package org.hyperion.hypercon.spec;

import java.util.Locale;

/**
 * The color tuning parameters of the different color channels (both in RGB space as in HSV space)
 */
public class ColorConfig {
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
	
	public boolean mSmoothingEnabled = false;
	/** The type of smoothing algorithm */
	public ColorSmoothingType mSmoothingType = ColorSmoothingType.linear;
	/** The time constant for smoothing algorithm in milliseconds */
	public int mSmoothingTime_ms = 200;
	/** The update frequency of the leds in Hz */
	public double mSmoothingUpdateFrequency_Hz = 20.0;
	
	/**
	 * Creates the JSON string of the configuration as used in the Hyperion daemon configfile
	 * 
	 * @return The JSON string of this ColorConfig
	 */
	public String toJsonString() {
		StringBuffer strBuf = new StringBuffer();
		
		strBuf.append("\t/// Color manipulation configuration used to tune the output colors to specific surroundings. Contains the following fields:\n");
		strBuf.append("\t///  * 'hsv' : The manipulation in the Hue-Saturation-Value color domain with the following tuning parameters:\n");
		strBuf.append("\t///            - 'saturationGain'  The gain adjustement of the saturation\n");
		strBuf.append("\t///            - 'valueGain'       The gain adjustement of the value\n");
		strBuf.append("\t///  * 'red'/'green'/'blue' : The manipulation in the Red-Green-Blue color domain with the following tuning parameters for each channel:\n");
		strBuf.append("\t///            - 'threshold'       The minimum required input value for the channel to be on (else zero)\n");
		strBuf.append("\t///            - 'gamma'           The gamma-curve correction factor\n");
		strBuf.append("\t///            - 'blacklevel'      The lowest possible value (when the channel is black)\n");
		strBuf.append("\t///            - 'whitelevel'      The highest possible value (when the channel is white)\n");
		strBuf.append("\t///  * 'smoothing' : Smoothing of the colors in the time-domain with the following tuning parameters:\n");
		strBuf.append("\t///            - 'type'            The type of smoothing algorithm ('linear' or 'none')\n");
		strBuf.append("\t///            - 'time_ms'         The time constant for smoothing algorithm in milliseconds\n");
		strBuf.append("\t///            - 'updateFrequency' The update frequency of the leds in Hz\n");

		strBuf.append("\t\"color\" :\n");
		strBuf.append("\t{\n");
		strBuf.append(hsvToJsonString() + ",\n");
		strBuf.append(rgbToJsonString() + ",\n");
		strBuf.append(smoothingToString() + "\n");
		strBuf.append("\t}");
		
		return strBuf.toString();
	}
	
	/**
	 * Creates the JSON string of the HSV-subconfiguration as used in the Hyperion deamon configfile
	 * 
	 * @return The JSON string of the HSV-config
	 */
	private String hsvToJsonString() {
		StringBuffer strBuf = new StringBuffer();
		strBuf.append("\t\t\"hsv\" :\n");
		strBuf.append("\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"saturationGain\" : %.4f,\n", mSaturationGain));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"valueGain\"      : %.4f\n", mValueGain));
		
		strBuf.append("\t\t}");
		return strBuf.toString();
	}
	
	/**
	 * Creates the JSON string of the RGB-subconfiguration as used in the Hyperion deamon configfile
	 * 
	 * @return The JSON string of the RGB-config
	 */
	private String rgbToJsonString() {
		StringBuffer strBuf = new StringBuffer();
		
		strBuf.append("\t\t\"red\" :\n");
		strBuf.append("\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"threshold\"  : %.4f,\n", mRedThreshold));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"gamma\"      : %.4f,\n", mRedGamma));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"blacklevel\" : %.4f,\n", mRedBlacklevel));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"whitelevel\" : %.4f\n",  mRedWhitelevel));
		strBuf.append("\t\t},\n");

		strBuf.append("\t\t\"green\" :\n");
		strBuf.append("\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"threshold\"  : %.4f,\n", mGreenThreshold));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"gamma\"      : %.4f,\n", mGreenGamma));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"blacklevel\" : %.4f,\n", mGreenBlacklevel));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"whitelevel\" : %.4f\n",  mGreenWhitelevel));
		strBuf.append("\t\t},\n");

		strBuf.append("\t\t\"blue\" :\n");
		strBuf.append("\t\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"threshold\"  : %.4f,\n", mBlueThreshold));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"gamma\"      : %.4f,\n", mBlueGamma));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"blacklevel\" : %.4f,\n", mBlueBlacklevel));
		strBuf.append(String.format(Locale.ROOT, "\t\t\t\"whitelevel\" : %.4f\n",  mBlueWhitelevel));
		strBuf.append("\t\t}");
		
		return strBuf.toString();
	}

	/**
	 * Creates the JSON string of the smoothing subconfiguration as used in the Hyperion deamon configfile
	 * 
	 * @return The JSON string of the HSV-config
	 */
	private String smoothingToString() {
		StringBuffer strBuf = new StringBuffer();
		
		String preamble = "\t\t";
		strBuf.append(preamble).append("\"smoothing\" :\n");
		strBuf.append(preamble).append("{\n");
		strBuf.append(preamble).append(String.format(Locale.ROOT, "\t\"type\"            : \"%s\",\n", (mSmoothingEnabled) ? mSmoothingType.name() : "none"));
		strBuf.append(preamble).append(String.format(Locale.ROOT, "\t\"time_ms\"         : %d,\n", mSmoothingTime_ms));
		strBuf.append(preamble).append(String.format(Locale.ROOT, "\t\"updateFrequency\" : %.4f\n", mSmoothingUpdateFrequency_Hz));
		
		strBuf.append(preamble).append("}");
		return strBuf.toString();
	}
}
