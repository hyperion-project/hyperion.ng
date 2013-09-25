package org.hyperion.config.spec;

import java.util.Locale;


public class MiscConfig {
	public BootSequence mBootSequence = BootSequence.rainbow;
	public int mBootSequenceLength_ms = 3000;
	
	public int mFrameGrabberWidth = 64;
	public int mFrameGrabberHeight = 64;
	public int mFrameGrabberInterval_ms = 100;

	public boolean mXbmcChecker = true;
	public String mXbmcAddress  = "127.0.0.1";
	public int mXbmcTcpPort     = 9090;
	public boolean mVideoOn = true;
	public boolean mMenuOn = false;
	public boolean mPictureOn = false;
	public boolean mAudioOn = false;
	
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
