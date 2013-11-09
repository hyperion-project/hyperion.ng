package org.hyperion.hypercon.spec;

import java.util.Locale;

/**
 * Miscellaneous configuration items for the Hyperion daemon.
 */
public class MiscConfig {
	
	/** Flag indicating that the boot sequence is enabled */
	public boolean mBootsequenceEnabled = true;
	/** The selected boot sequence */
	public BootSequence mBootSequence = BootSequence.rainbow;
	/** The length of the boot sequence [ms] */
	public int mBootSequenceLength_ms = 3000;
	
	/** Flag indicating that the Frame Grabber is enabled */
	public boolean mFrameGrabberEnabled = true;
	/** The width of 'grabbed' frames (screen shots) [pixels] */
	public int mFrameGrabberWidth = 64;
	/** The height of 'grabbed' frames (screen shots) [pixels] */
	public int mFrameGrabberHeight = 64;
	/** The interval of frame grabs (screen shots) [ms] */
	public int mFrameGrabberInterval_ms = 100;

	/** Flag indicating that the XBMC checker is enabled */
	public boolean mXbmcCheckerEnabled = true;
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

	/** Flag indicating that the JSON interface is enabled */
	public boolean mJsonInterfaceEnabled = true;
	/** The TCP port at which the JSON server is listening for incoming connections */
	public int mJsonPort = 19444;

	/** Flag indicating that the PROTO interface is enabled */
	public boolean mProtoInterfaceEnabled = true;
	/** The TCP port at which the Protobuf server is listening for incoming connections */
	public int mProtoPort = 19445;

	/** Flag indicating that the PROTO interface is enabled */
	public boolean mBoblightInterfaceEnabled = false;
	/** The TCP port at which the Protobuf server is listening for incoming connections */
	public int mBoblightPort = 19333;
	
	/**
	 * Creates the JSON string of the configuration as used in the Hyperion daemon configfile
	 * 
	 * @return The JSON string of this MiscConfig
	 */
	public String toJsonString() {
		StringBuffer strBuf = new StringBuffer();

		strBuf.append("\n\n");
		strBuf.append("\t/// The boot-sequence configuration, contains the following items: \n");
		strBuf.append("\t///  * type        : The type of the boot-sequence ('rainbow', 'knight_rider', 'none') \n");
		strBuf.append("\t///  * duration_ms : The length of the boot-sequence [ms]\n");
		
		String bootPreamble = mBootsequenceEnabled? "\t" : "//\t";
		strBuf.append(bootPreamble).append("\"bootsequence\" :\n");
		strBuf.append(bootPreamble).append("{\n");
		strBuf.append(bootPreamble).append(String.format(Locale.ROOT, "\t\"type\"        : \"%s\",\n", mBootSequence));
		strBuf.append(bootPreamble).append(String.format(Locale.ROOT, "\t\"duration_ms\" : %d\n", mBootSequenceLength_ms));
		strBuf.append(bootPreamble).append("},\n\n");

		strBuf.append("\n\n");
		strBuf.append("\t/// The configuration for the frame-grabber, contains the following items: \n");
		strBuf.append("\t///  * width        : The width of the grabbed frames [pixels]\n");
		strBuf.append("\t///  * height       : The height of the grabbed frames [pixels]\n");
		strBuf.append("\t///  * frequency_Hz : The frequency of the frame grab [Hz]\n");
		
		String grabPreamble = mFrameGrabberEnabled? "\t" : "//\t";
		strBuf.append(grabPreamble).append("\"framegrabber\" :\n");
		strBuf.append(grabPreamble).append("{\n");
		strBuf.append(grabPreamble).append(String.format(Locale.ROOT, "\t\"width\"        : %d,\n", mFrameGrabberWidth));
		strBuf.append(grabPreamble).append(String.format(Locale.ROOT, "\t\"height\"       : %d,\n", mFrameGrabberHeight));
		strBuf.append(grabPreamble).append(String.format(Locale.ROOT, "\t\"frequency_Hz\" : %.1f\n", 1000.0/mFrameGrabberInterval_ms));
		strBuf.append(grabPreamble).append("},\n\n");

		strBuf.append("\n\n");
		strBuf.append("\t/// The configuration of the XBMC connection used to enable and disable the frame-grabber. Contains the following fields: \n");
		strBuf.append("\t///  * xbmcAddress  : The IP address of the XBMC-host\n");
		strBuf.append("\t///  * xbmcTcpPort  : The TCP-port of the XBMC-server\n");
		strBuf.append("\t///  * grabVideo    : Flag indicating that the frame-grabber is on(true) during video playback\n");
		strBuf.append("\t///  * grabPictures : Flag indicating that the frame-grabber is on(true) during picture show\n");
		strBuf.append("\t///  * grabAudio    : Flag indicating that the frame-grabber is on(true) during audio playback\n");
		strBuf.append("\t///  * grabMenu     : Flag indicating that the frame-grabber is on(true) in the XBMC menu\n");

		String xbmcPreamble = mXbmcCheckerEnabled? "\t" : "//\t";
		strBuf.append(xbmcPreamble).append("\"xbmcVideoChecker\" :\n");
		strBuf.append(xbmcPreamble).append("{\n");
		strBuf.append(xbmcPreamble).append(String.format(Locale.ROOT, "\t\"xbmcAddress\"  : \"%s\",\n", mXbmcAddress));
		strBuf.append(xbmcPreamble).append(String.format(Locale.ROOT, "\t\"xbmcTcpPort\"  : %d,\n", mXbmcTcpPort));
		strBuf.append(xbmcPreamble).append(String.format(Locale.ROOT, "\t\"grabVideo\"    : %s,\n", mVideoOn));
		strBuf.append(xbmcPreamble).append(String.format(Locale.ROOT, "\t\"grabPictures\" : %s,\n", mPictureOn));
		strBuf.append(xbmcPreamble).append(String.format(Locale.ROOT, "\t\"grabAudio\"    : %s,\n", mAudioOn));
		strBuf.append(xbmcPreamble).append(String.format(Locale.ROOT, "\t\"grabMenu\"     : %s\n", mMenuOn));
		strBuf.append(xbmcPreamble).append("},\n\n");

		
		strBuf.append("\t/// The configuration of the Json server which enables the json remote interface\n");
		strBuf.append("\t///  * port : Port at which the json server is started\n");
		
		String jsonPreamble = mJsonInterfaceEnabled? "\t" : "//\t";
		strBuf.append(jsonPreamble).append("\"jsonServer\" :\n");
		strBuf.append(jsonPreamble).append("{\n");
		strBuf.append(jsonPreamble).append(String.format(Locale.ROOT, "\t\"port\" : %d\n", mJsonPort));
	    strBuf.append(jsonPreamble).append("},\n\n");


	    strBuf.append("\t/// The configuration of the Proto server which enables the protobuffer remote interface\n");
	    strBuf.append("\t///  * port : Port at which the protobuffer server is started\n");
	    
	    String protoPreamble = mProtoInterfaceEnabled? "\t" : "//\t";
	    strBuf.append(protoPreamble).append("\"protoServer\" :\n");
	    strBuf.append(protoPreamble).append("{\n");
		strBuf.append(protoPreamble).append(String.format(Locale.ROOT, "\t\"port\" : %d\n", mProtoPort));
	    strBuf.append(protoPreamble).append("},\n\n");
	    

	    strBuf.append("\t/// The configuration of the boblight server which enables the boblight remote interface\n");
	    strBuf.append("\t///  * port : Port at which the boblight server is started\n");
	    
	    String bobligthPreamble = mBoblightInterfaceEnabled? "\t" : "//\t";
	    strBuf.append(bobligthPreamble).append("\"boblightServer\" :\n");
	    strBuf.append(bobligthPreamble).append("{\n");
		strBuf.append(bobligthPreamble).append(String.format(Locale.ROOT, "\t\"port\" : %d\n", mBoblightPort));
	    strBuf.append(bobligthPreamble).append("},\n\n");

	    strBuf.append("\t\"end-of-json\" : \"end-of-json\"");
	    
	    return strBuf.toString();
	}
}
