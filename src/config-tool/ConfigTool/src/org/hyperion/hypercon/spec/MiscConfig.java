package org.hyperion.hypercon.spec;

import java.util.Locale;

/**
 * Miscellaneous configuration items for the Hyperion daemon.
 */
public class MiscConfig {
	/** The selected boot sequence */
	public BootSequence mBootSequence = BootSequence.rainbow;
	/** The length of the boot sequence [ms] */
	public int mBootSequenceLength_ms = 3000;
	
	/** The width of 'grabbed' frames (screen shots) [pixels] */
	public int mFrameGrabberWidth = 64;
	/** The height of 'grabbed' frames (screen shots) [pixels] */
	public int mFrameGrabberHeight = 64;
	/** The interval of frame grabs (screen shots) [ms] */
	public int mFrameGrabberInterval_ms = 100;

	/** Flag enabling/disabling XBMC communication */
	public boolean mXbmcChecker = true;
	/** The IP-address of XBMC */
	public String mXbmcAddress  = "127.0.0.1";
	/** The TCP JSON-Port of XBMC */
	public int mXbmcTcpPort     = 9090;
	/** Flag indicating that the frame-grabber is on during video playback */
	public boolean mVideoOn = true;
	/** Flag indicating that the frame-grabber is on during XBMC menu */
	public boolean mMenuOn = false;
	/** Flag indicating that the frame-grabber is on during picture slideshow */
	public boolean mPictureOn = false;
	/** Flag indicating that the frame-grabber is on during audio playback */
	public boolean mAudioOn = false;
	
	/**
	 * Creates the JSON string of the configuration as used in the Hyperion daemon configfile
	 * 
	 * @return The JSON string of this MiscConfig
	 */
	public String toJsonString() {
		StringBuffer strBuf = new StringBuffer();
		strBuf.append("\t\"bootsequence\" :\n");
		strBuf.append("\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\"type\"        : \"%s\",\n", mBootSequence));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"duration_ms\" : %d\n", mBootSequenceLength_ms));
		strBuf.append("\t},\n");
		
		strBuf.append("\t\"framegrabber\" :\n");
		strBuf.append("\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\"width\"        : %d,\n", mFrameGrabberWidth));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"height\"       : %d,\n", mFrameGrabberHeight));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"frequency_Hz\" : %.1f\n", 1000.0/mFrameGrabberInterval_ms));
		strBuf.append("\t},\n");

		strBuf.append("\t\"xbmcVideoChecker\" :\n");
		strBuf.append("\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\"enable\"      : %s,\n", mXbmcChecker));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"xbmcAddress\" : \"%s\",\n", mXbmcAddress));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"xbmcTcpPort\" : %d\n", mXbmcTcpPort));
		strBuf.append("\t}");
		
		return strBuf.toString();
	}


}
