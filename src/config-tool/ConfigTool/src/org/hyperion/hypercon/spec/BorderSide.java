package org.hyperion.hypercon.spec;

/**
 * Enumeration of possible led-locations (aka border-sides). This also contains the specification of
 * the angle at which the led is placed along a specific border (0.0rad = pointing right).
 */
public enum BorderSide {
	top_left (0.75*Math.PI),
	top(0.5*Math.PI),
	top_right(0.25*Math.PI),
	right(0.0*Math.PI),
	bottom_right(-0.25*Math.PI),
	bottom(-0.5*Math.PI),
	bottom_left(-0.75*Math.PI),
	left(1.0*Math.PI);
	
	/** The angle of the led [rad] */
	private final double mAngle_rad;
	
	/**
	 * Constructs the BorderSide with the given led angle
	 * 
	 * @param pAngle_rad  The angle of the led [rad]
	 */
	BorderSide(double pAngle_rad) {
		mAngle_rad = pAngle_rad;
	}
	
	/**
	 * Returns the angle of the led placement
	 * 
	 * @return The angle of the led [rad]
	 */
	public double getAngle_rad() {
		return mAngle_rad;
	}
}
