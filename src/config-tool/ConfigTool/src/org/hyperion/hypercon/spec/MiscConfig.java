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

	/** The IP-address of XBMC */
	public String mXbmcAddress  = "127.0.0.1";
	/** The TCP JSON-Port of XBMC */
	public int mXbmcTcpPort     = 9090;
	/** Flag indicating that the frame-grabber is on during video playback */
	public boolean mVideoOn = true;
	/** Flag indicating that the frame-grabber is on during XBMC menu */
	public boolean mMenuOn = false;
	/** Flag indicating that the frame-grabber is on during picture slideshow */
	public boolean mPictureOn = true;
	/** Flag indicating that the frame-grabber is on during audio playback */
	public boolean mAudioOn = true;

	/** The TCP port at which the JSON server is listening for incoming connections */
	public int mJsonPort = 19444;

	/** The TCP port at which the Protobuf server is listening for incoming connections */
	public int mProtoPort = 19445;
	
	/**
	 * Creates the JSON string of the configuration as used in the Hyperion daemon configfile
	 * 
	 * @return The JSON string of this MiscConfig
	 */
	public String toJsonString() {
		StringBuffer strBuf = new StringBuffer();

		strBuf.append("\t/// The boot-sequence configuration, contains the following items: \n");
		strBuf.append("\t///  * type        : The type of the boot-sequence ('rainbow', 'knight_rider', 'none') \n");
		strBuf.append("\t///  * duration_ms : The length of the boot-sequence [ms]\n");
		
		strBuf.append("\t\"bootsequence\" :\n");
		strBuf.append("\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\"type\"        : \"%s\",\n", mBootSequence));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"duration_ms\" : %d\n", mBootSequenceLength_ms));
		strBuf.append("\t},\n\n");

		
		strBuf.append("\t/// The configuration for the frame-grabber, contains the following items: \n");
		strBuf.append("\t///  * width        : The width of the grabbed frames [pixels]\n");
		strBuf.append("\t///  * height       : The height of the grabbed frames [pixels]\n");
		strBuf.append("\t///  * frequency_Hz : The frequency of the frame grab [Hz]\n");
		
		strBuf.append("\t\"framegrabber\" :\n");
		strBuf.append("\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\"width\"        : %d,\n", mFrameGrabberWidth));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"height\"       : %d,\n", mFrameGrabberHeight));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"frequency_Hz\" : %.1f\n", 1000.0/mFrameGrabberInterval_ms));
		strBuf.append("\t},\n\n");

		
		strBuf.append("\t/// The configuration of the XBMC connection used to enable and disable the frame-grabber. Contains the following fields: \n");
		strBuf.append("\t///  * xbmcAddress  : The IP address of the XBMC-host\n");
		strBuf.append("\t///  * xbmcTcpPort  : The TCP-port of the XBMC-server\n");
		strBuf.append("\t///  * grabVideo    : Flag indicating that the frame-grabber is on(true) during video playback\n");
		strBuf.append("\t///  * grabPictures : Flag indicating that the frame-grabber is on(true) during picture show\n");
		strBuf.append("\t///  * grabAudio    : Flag indicating that the frame-grabber is on(true) during audio playback\n");
		strBuf.append("\t///  * grabMenu     : Flag indicating that the frame-grabber is on(true) in the XBMC menu\n");
		
		strBuf.append("\t\"xbmcVideoChecker\" :\n");
		strBuf.append("\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\"xbmcAddress\"  : \"%s\",\n", mXbmcAddress));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"xbmcTcpPort\"  : %d,\n", mXbmcTcpPort));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"grabVideo\"    : %s,\n", mVideoOn));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"grabPictures\" : %s,\n", mPictureOn));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"grabAudio\"    : %s,\n", mAudioOn));
		strBuf.append(String.format(Locale.ROOT, "\t\t\"grabMenu\"     : %s\n", mMenuOn));
		strBuf.append("\t},\n\n");
		
				
		strBuf.append("\t/// The configuration of the Json server which enables the json remote interface\n");
		strBuf.append("\t///  * port : Port at which the json server is started\n");
		strBuf.append("\t\"jsonServer\" :\n");
		strBuf.append("\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\"port\" : %d\n", mJsonPort));
	    strBuf.append("\t},\n\n");


	    strBuf.append("\t/// The configuration of the Proto server which enables the protobuffer remote interface\n");
	    strBuf.append("\t///  * port : Port at which the protobuffer server is started\n");
	    
	    strBuf.append("\t\"protoServer\" :\n");
	    strBuf.append("\t{\n");
		strBuf.append(String.format(Locale.ROOT, "\t\t\"port\" : %d\n", mProtoPort));
	    strBuf.append("\t}");
	    
		return strBuf.toString();
	}
}
