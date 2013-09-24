package org.hyperion.config.spec;

public enum BorderSide {
	top_left (0.75*Math.PI),
	top(0.5*Math.PI),
	top_right(0.25*Math.PI),
	right(0.0*Math.PI),
	bottom_right(-0.25*Math.PI),
	bottom(-0.5*Math.PI),
	bottom_left(-0.75*Math.PI),
	left(1.0*Math.PI);
	
	private final double mAngle;
	
	BorderSide(double pAngle) {
		mAngle = pAngle;
	}
	
	public double getAngle() {
		return mAngle;
	}
}
