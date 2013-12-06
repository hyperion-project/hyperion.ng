package org.hyperion.hypercon.spec;

import java.util.Vector;

import org.hyperion.hypercon.JsonStringBuffer;

/**
 * The configuration structure for a single 'effect'. 
 */
public class EffectConfig {

	/** The identifier of the effect */
	public String mId;
	
	/** The python-script used to generate the effect */
	public String mScript;
	
	/** The arguments (key-value) of the python-script */
	public final Vector<EffectArg> mArgs = new Vector<>();
	
	static public class EffectArg {
		public String key;
		public Object value;
		
		public EffectArg() {
			key = "";
			value = "";
		}
		public EffectArg(String pKey, Object pValue) {
			key = pKey;
			value = pValue;
		}
	}
	
	public void append(JsonStringBuffer pJsonBuf, boolean endOfEffects) {
		pJsonBuf.startObject(mId);
		pJsonBuf.addValue("script", mScript, false);
		
		//pJsonBuf.addRawValue("args", String.format(Locale.ENGLISH, "{\n%s\n}", mArgs), true);
		
		pJsonBuf.stopObject(endOfEffects);
	}
	
	@Override
	public String toString() {
		return mId;
	}
	
	@Override
	public EffectConfig clone() {
		EffectConfig thisClone = new EffectConfig();
		thisClone.mId     = mId;
		thisClone.mScript = mScript;
		
		for (EffectArg arg : mArgs) {
			thisClone.mArgs.add(new EffectArg(arg.key, arg.value));
		}
		
		return thisClone;
	}
}
