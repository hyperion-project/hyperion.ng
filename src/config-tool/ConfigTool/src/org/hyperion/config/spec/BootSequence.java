package org.hyperion.config.spec;

public enum BootSequence {
	rainbow,
	knight_rider,
	none;
	
	public static BootSequence fromString(String pStr) {
		for (BootSequence seq : values()) {
			if (seq.toString().equalsIgnoreCase(pStr)) {
				return seq;
			}
		}
		return none;
	}
	
	@Override
	public String toString() {
		switch(this) {
		case rainbow:
			return "Rainbow";
		case knight_rider:
			return "Kinght Rider";
		case none:
			return "None";
		}
		return "None";
	}
}
