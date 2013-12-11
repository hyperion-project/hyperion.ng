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
	
	/**
	 * The effect argument contains a key-value combination that holds a single argument of an 
	 * effect
	 */
	static public class EffectArg {
		/** The key of the effect argument */
		public String key;
		/** The value of the effect argument */
		public Object value;
		
		/**
		 * Constructs an new effect argument with empty key and value
		 */
		public EffectArg() {
			this("", "");
		}
		
		/**
		 * Constructs an effect argument with the given key and value
		 *  
		 * @param pKey The key of the new argument
		 * @param pValue The value of the new argument
		 */
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
