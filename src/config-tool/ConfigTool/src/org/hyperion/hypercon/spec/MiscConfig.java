package org.hyperion.hypercon.spec;

import org.hyperion.hypercon.JsonStringBuffer;

/**
 * Miscellaneous configuration items for the Hyperion daemon.
 */
public class MiscConfig {

	/** The absolute location(s) of the effects */
	public String mEffectEnginePath = "/opt/hyperion/effects";
	
	/** Flag indicating that the boot sequence is enabled(true) or not(false) */
	public boolean mBootSequenceEnabled = true;
	/** The effect selected for the boot sequence */
	public String mBootSequenceEffect = "Rainbow Swirl (fast)";
	
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
	
	public void appendTo(JsonStringBuffer strBuf) {
		String effectEngineComment = 
				"The configuration of the effect engine, contains the following items: \n" +
				" * paths        : An array with absolute location(s) of directories with effects \n" +
				" * bootsequence : The effect selected as 'boot sequence'";
		strBuf.writeComment(effectEngineComment);
		
		String[] effectPaths = mEffectEnginePath.split(":");
		
		strBuf.startObject("effects");
		strBuf.startArray("paths");
		for (String effectPath : effectPaths) {
			strBuf.addArrayElement(effectPath, effectPath == effectPaths[effectPaths.length-1]);
		}
		strBuf.stopArray();
		strBuf.toggleComment(!mBootSequenceEnabled);
		strBuf.addValue("bootsequence", mBootSequenceEffect,  true);
		strBuf.toggleComment(false);
		strBuf.stopObject();

		strBuf.newLine();
		
		String grabComment =
				" The configuration for the frame-grabber, contains the following items: \n" +
				"  * width        : The width of the grabbed frames [pixels]\n" +
				"  * height       : The height of the grabbed frames [pixels]\n" +
				"  * frequency_Hz : The frequency of the frame grab [Hz]\n";
		strBuf.writeComment(grabComment);
		
		strBuf.toggleComment(!mFrameGrabberEnabled);
		strBuf.startObject("framegrabber");
		strBuf.addValue("width", mFrameGrabberWidth, false);
		strBuf.addValue("height", mFrameGrabberHeight, false);
		strBuf.addValue("frequency_Hz", 1000.0/mFrameGrabberInterval_ms, true);
		strBuf.stopObject();
		strBuf.toggleComment(false);
		
		strBuf.newLine();
		
		String xbmcComment = 
				"The configuration of the XBMC connection used to enable and disable the frame-grabber. Contains the following fields: \n" +
				" * xbmcAddress  : The IP address of the XBMC-host\n" +
				" * xbmcTcpPort  : The TCP-port of the XBMC-server\n" +
				" * grabVideo    : Flag indicating that the frame-grabber is on(true) during video playback\n" +
				" * grabPictures : Flag indicating that the frame-grabber is on(true) during picture show\n" +
				" * grabAudio    : Flag indicating that the frame-grabber is on(true) during audio playback\n" +
				" * grabMenu     : Flag indicating that the frame-grabber is on(true) in the XBMC menu\n";
		strBuf.writeComment(xbmcComment);
		
		strBuf.toggleComment(!mXbmcCheckerEnabled);
		strBuf.startObject("xbmcVideoChecker");
		strBuf.addValue("xbmcAddress", mXbmcAddress, false);
		strBuf.addValue("xbmcTcpPort", mXbmcTcpPort, false);
		strBuf.addValue("grabVideo", mVideoOn, false);
		strBuf.addValue("grabPictures", mPictureOn, false);
		strBuf.addValue("grabAudio", mAudioOn, false);
		strBuf.addValue("grabMenu", mMenuOn, true);
		strBuf.stopObject();
		strBuf.toggleComment(false);

		strBuf.newLine();

		String jsonComment = 
				"The configuration of the Json server which enables the json remote interface\n" +
				" * port : Port at which the json server is started\n";
		strBuf.writeComment(jsonComment);
		
		strBuf.toggleComment(!mJsonInterfaceEnabled);
		strBuf.startObject("jsonServer");
		strBuf.addValue("port", mJsonPort, true);
		strBuf.stopObject();
		strBuf.toggleComment(false);

		strBuf.newLine();
		
		String protoComment =
			    "The configuration of the Proto server which enables the protobuffer remote interface\n" +
			    " * port : Port at which the protobuffer server is started\n";
		strBuf.writeComment(protoComment);
		
		strBuf.toggleComment(!mProtoInterfaceEnabled);
		strBuf.startObject("protoServer");
		strBuf.addValue("port", mProtoPort, true);
		strBuf.stopObject();
		strBuf.toggleComment(false);

		strBuf.newLine();
	    
		String boblightComment =
			    "The configuration of the boblight server which enables the boblight remote interface\n" +
			    " * port : Port at which the boblight server is started\n";
		strBuf.writeComment(boblightComment);
		
		strBuf.toggleComment(!mBoblightInterfaceEnabled);
		strBuf.startObject("boblightServer");
		strBuf.addValue("port", mBoblightPort, true);
		strBuf.stopObject();
		strBuf.toggleComment(false);
	}
	/**
	 * Creates the JSON string of the configuration as used in the Hyperion daemon configfile
	 * 
	 * @return The JSON string of this MiscConfig
	 */
	public String toJsonString() {
		JsonStringBuffer jsonBuf = new JsonStringBuffer(1);
		appendTo(jsonBuf);
		return jsonBuf.toString();
	}
	
	public static void main(String[] pArgs) {
		MiscConfig miscConfig = new MiscConfig();
		
		JsonStringBuffer jsonBuf = new JsonStringBuffer(1);
		miscConfig.appendTo(jsonBuf);
		
		System.out.println(jsonBuf.toString());
	}
}
