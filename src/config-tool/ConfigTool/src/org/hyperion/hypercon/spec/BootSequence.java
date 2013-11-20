package org.hyperion.hypercon.spec;

/**
 * Enumeration of possible boot sequences
 */
public enum BootSequence {
	/** The rainbow boot sequence */ 
	rainbow,
	/** The Knight Rider (or KITT) boot sequence */
	knightrider;
	
	/**
	 * Returns a string representation of the BootSequence
	 * 
	 * @return String representation of this boot-sequence
	 */
	@Override
	public String toString() {
		switch(this) {
		case rainbow:
			return "Rainbow";
		case knightrider:
			return "Knight Rider";
		}
		return "None";
	}
}
