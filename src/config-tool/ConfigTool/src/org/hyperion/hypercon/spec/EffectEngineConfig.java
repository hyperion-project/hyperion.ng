package org.hyperion.hypercon.spec;

import java.util.Vector;

import org.hyperion.hypercon.JsonStringBuffer;

/**
 * The configuration structure for the effect engine. It contains definition for zero or more 
 * 'effects' 
 */
public class EffectEngineConfig {

	public final Vector<EffectConfig> mEffects = new Vector<>();
	
	public void appendTo(JsonStringBuffer pJsonBuf) {
		pJsonBuf.startObject("effects");
		
		for (EffectConfig effect : mEffects) {
			effect.append(pJsonBuf, effect.equals(mEffects.get(mEffects.size()-1)));
		}
		
		pJsonBuf.stopObject();
	}

	public void loadDefault() {
		EffectConfig rainbowSwirl = new EffectConfig();
		rainbowSwirl.mId = "Rainbow swirl";
		rainbowSwirl.mScript = "rainbow-swirl.py";
		rainbowSwirl.mArgs.add(new EffectConfig.EffectArg("rotation-time", 10.0)); 
		rainbowSwirl.mArgs.add(new EffectConfig.EffectArg("brightness", 1.0)); 
		rainbowSwirl.mArgs.add(new EffectConfig.EffectArg("reverse", false)); 
				
		EffectConfig rainbowMood = new EffectConfig();
		rainbowMood.mId = "Rainbow mood";
		rainbowMood.mScript = "rainbow-mood.py";
		rainbowMood.mArgs.add(new EffectConfig.EffectArg("rotation-time", 10.0)); 
		rainbowMood.mArgs.add(new EffectConfig.EffectArg("brightness", 1.0)); 
		rainbowMood.mArgs.add(new EffectConfig.EffectArg("reverse", false)); 
		
		mEffects.add(rainbowSwirl);
		mEffects.add(rainbowMood);
	}
}
