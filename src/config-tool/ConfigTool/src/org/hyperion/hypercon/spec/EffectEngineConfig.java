package org.hyperion.hypercon.spec;

import java.util.Vector;

import org.hyperion.hypercon.JsonStringBuffer;

/**
 * The configuration structure for the effect engine. It contains definition for zero or more 
 * 'effects' 
 */
public class EffectEngineConfig {

	public final Vector<EffectConfig> mEffects = new Vector<>();
	{
		EffectConfig testSlow = new EffectConfig();
		testSlow.mId = "test-slow";
		testSlow.mScript = "test.py";
		testSlow.mArgs = "\"speed\" : 0.5";
		
		EffectConfig testFast = new EffectConfig();
		testFast.mId = "test-fast";
		testFast.mScript = "test.py";
		testFast.mArgs = "\"speed\" : 2.0";
		
		EffectConfig rainbowSwirl = new EffectConfig();
		rainbowSwirl.mId = "Rainbow swirl";
		rainbowSwirl.mScript = "rainbow-swirl.py";
		rainbowSwirl.mArgs = 
				"\"rotation-time\" : 10.0,\n" +
				"\"brightness\" : 1.0,\n" +
				"\"reverse\" : false";
				
		EffectConfig rainbowMood = new EffectConfig();
		rainbowMood.mId = "Rainbow mood";
		rainbowMood.mScript = "rainbow-mood.py";
		rainbowMood.mArgs = 
				"\"rotation-time\" : 10.0,\n" +
				"\"brightness\" : 1.0,\n" +
				"\"reverse\" : false";
		
		mEffects.add(testSlow);
		mEffects.add(testFast);
		mEffects.add(rainbowSwirl);
		mEffects.add(rainbowMood);
	}
	
	public void appendTo(JsonStringBuffer pJsonBuf) {
		pJsonBuf.startObject("effects");
		
		for (EffectConfig effect : mEffects) {
			effect.append(pJsonBuf, effect.equals(mEffects.get(mEffects.size()-1)));
		}
		
		pJsonBuf.stopObject();
	}
}
