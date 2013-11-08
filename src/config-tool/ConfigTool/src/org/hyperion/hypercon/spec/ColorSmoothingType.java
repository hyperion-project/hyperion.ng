package org.hyperion.hypercon.spec;

public enum ColorSmoothingType {
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