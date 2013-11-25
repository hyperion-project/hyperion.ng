package org.hyperion.hypercon.spec;

import java.util.List;
import java.util.Locale;
import java.util.Vector;

/**
 * The color tuning parameters of the different color channels (both in RGB space as in HSV space)
 */
public class ColorConfig {
	
	/** List with color transformations */
	public List<TransformConfig> mTransforms = new Vector<>();
	{
		mTransforms.add(new TransformConfig());
	}
	
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
		
		strBuf.append("\t\t\"transform\" :\n");
		strBuf.append("\t\t[\n");
		for (int i=0; i<mTransforms.size(); ++i) {
			TransformConfig transform = mTransforms.get(i);
			strBuf.append(transform.toJsonString());
			if (i == mTransforms.size()-1) {
				strBuf.append("\n");
			} else {
				strBuf.append(",\n");
			}
		}
		strBuf.append("\t\t],\n");

		strBuf.append(smoothingToString() + "\n");
		strBuf.append("\t}");
		
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
