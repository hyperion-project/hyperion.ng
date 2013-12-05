package org.hyperion.hypercon.spec;

import java.util.Locale;

import org.hyperion.hypercon.JsonStringBuffer;

/**
 * The configuration structure for a single 'effect'. 
 */
public class EffectConfig {

	/** The identifier of the effect */
	public String mId;
	
	/** The python-script used to generate the effect */
	public String mScript;
	
	/** The JSON-string containing the arguments of the python-script */
	public String mArgs;
	
	public void append(JsonStringBuffer pJsonBuf, boolean endOfEffects) {
		pJsonBuf.startObject(mId);
		pJsonBuf.addValue("script", mScript, false);
		
		pJsonBuf.addRawValue("args", String.format(Locale.ENGLISH, "{\n%s\n}", mArgs), true);
		
		pJsonBuf.stopObject(endOfEffects);
	}
	
	@Override
	public String toString() {
		return mId;
	}
}
