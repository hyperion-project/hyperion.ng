package org.hyperion.hypercon.gui.effectengine;

import java.awt.Color;

/**
 * Enumeration for the type of possible effect-arguments
 */
public enum EffectArgumentType {
	/** The 'int' instance of the argument-type, maps to 'int' or {@link java.lang.Integer} */
	int_arg("int"),
	/** The 'double' instance of the argument-type, maps to 'double' or {@link java.lang.Double} */
	double_arg("double"),
	/** The 'color' instance of the argument-type, maps to {@link java.awt.Color} */
	color_arg("color"),
	/** The 'string' instance of the argument-type, maps to {@link java.lang.String} */
	string_arg("string");
	
	/** The 'pretty' name of the effect argument type (should be unique) */
	private final String mName;
	
	/**
	 * Constructs the EffectArgumentType with the given name
	 * 
 	 * @param pName The name of the type
	 */
	private EffectArgumentType(final String pName) {
		mName = pName;
	}
	
	public Object getDefaultValue() {
		switch(this) {
		case int_arg:
			return 0;
		case double_arg:
			return 0.0;
		case color_arg:
			return Color.WHITE;
		case string_arg:
			return "";
		}
		return "";
	}
	
	/**
	 * Returns the string representation of the argument-type, which is its set name
	 * 
	 * @return The name of the EffectArgumentType
	 */
	@Override
	public String toString() {
		return mName;
	}
}
