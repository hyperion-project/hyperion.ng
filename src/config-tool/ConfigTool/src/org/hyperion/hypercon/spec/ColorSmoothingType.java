package org.hyperion.hypercon.spec;

public enum ColorSmoothingType {
	/** No smoothing in the time domain */
	none("None"),
	
	/** Linear smoothing of led data */
	linear("Linear smoothing");
	
	private final String mName;
	
	private ColorSmoothingType(String name) {
		mName = name;
	}
	
	@Override
	public String toString() {
		return mName;
	}
}