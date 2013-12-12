package org.hyperion.hypercon;

public class JsonStringBuffer {

	private final StringBuffer mStrBuf = new StringBuffer();
	
	private final int mStartIndentLevel;
	private int mIndentLevel = 0;
	
	/** Flag indicating that the parts written are 'commented-out' */
	private boolean mComment = false;
	
	public JsonStringBuffer() {
		this(0);

		mStrBuf.append("{\n");
		++mIndentLevel;
	}
	
	public JsonStringBuffer(int pIndentLevel) {
		mStartIndentLevel = pIndentLevel;
		mIndentLevel      = pIndentLevel;
	}

	public void newLine() {
		mStrBuf.append('\n');
	}
	
	public void finish() {
		
		for (int i=0; i<mIndentLevel; ++i) {
			mStrBuf.append('\t');
		}
		mStrBuf.append("\"end-of-json\" : \"end-of-json\"\n");
		
		--mIndentLevel;
		if (mIndentLevel != mStartIndentLevel) {
			System.err.println("Json write closed in incorrect state!");
		}
		for (int i=0; i<mIndentLevel; ++i) {
			mStrBuf.append('\t');
		}
		mStrBuf.append("}\n");
	}
	
	public void writeComment(String pComment) {
		String[] commentLines = pComment.split("\\r?\\n");

		for (String commentLine : commentLines) {
			for (int i=0; i<mIndentLevel; ++i) {
				mStrBuf.append('\t');
			}
			mStrBuf.append("/// ").append(commentLine).append('\n');
		}
	}
	
	public void toggleComment(boolean b) {
		mComment = b;
	}
	
	private void startLine() {
		if (mComment) mStrBuf.append("// ");
		for (int i=0; i<mIndentLevel; ++i) {
			mStrBuf.append('\t');
		}
	}
	
	public void startObject(String pKey) {
		if (!pKey.isEmpty()) {
			startLine();
			mStrBuf.append('"').append(pKey).append('"').append(" : \n");
		}
		startLine();
		mStrBuf.append("{\n");
		
		++mIndentLevel;
	}
	
	public void stopObject(boolean endOfSection) {
		--mIndentLevel;

		startLine();
		if (endOfSection) {
			mStrBuf.append("}\n");
		} else {
			mStrBuf.append("},\n");
		}
		
	}
	
	public void stopObject() {
		--mIndentLevel;

		startLine();
		mStrBuf.append("},\n");
	}
	
	public void startArray(String pKey) {
		startLine();
		mStrBuf.append('"').append(pKey).append('"').append(" : \n");
		startLine();
		mStrBuf.append("[\n");
		
		++mIndentLevel;
	}
	
	public void stopArray(boolean lastValue) {
		--mIndentLevel;

		startLine();
		if (lastValue) {
			mStrBuf.append("]\n");
		} else {
			mStrBuf.append("],\n");
		}
	}
	
	
	public void addRawValue(String pKey, String pValue, boolean lastValue) {
		startLine();
		mStrBuf.append('"').append(pKey).append('"').append(" : ").append(pValue);
		if (lastValue) {
			mStrBuf.append("\n");
		} else {
			mStrBuf.append(",\n");
		}
	}
	
	public void addValue(String pKey, String pValue, boolean lastValue) {
		startLine();
		mStrBuf.append('"').append(pKey).append('"').append(" : ").append('"').append(pValue).append('"');
		if (lastValue) {
			mStrBuf.append("\n");
		} else {
			mStrBuf.append(",\n");
		}
	}
	
	public void addValue(String pKey, double pValue, boolean lastValue) {
		startLine();
		mStrBuf.append('"').append(pKey).append('"').append(" : ").append(pValue);
		if (lastValue) {
			mStrBuf.append("\n");
		} else {
			mStrBuf.append(",\n");
		}
	}
	
	public void addValue(String pKey, int pValue, boolean lastValue) {
		startLine();
		mStrBuf.append('"').append(pKey).append('"').append(" : ").append(pValue);
		if (lastValue) {
			mStrBuf.append("\n");
		} else {
			mStrBuf.append(",\n");
		}
	}
	
	public void addValue(String pKey, boolean pValue, boolean lastValue) {
		startLine();
		mStrBuf.append('"').append(pKey).append('"').append(" : ").append(pValue);
		if (lastValue) {
			mStrBuf.append("\n");
		} else {
			mStrBuf.append(",\n");
		}
	}
	
	/**
	 * Adds an array element to an opened array. 
	 * 
	 * @param pValue The value of the element
	 * @param pLastValue Indicates that it is the last element in the array
	 */
	public void addArrayElement(String pValue, boolean pLastValue) {
		startLine();
		mStrBuf.append('"').append(pValue).append('"');
		if (pLastValue) {
			mStrBuf.append("\n");
		} else {
			mStrBuf.append(",\n");
		}
	}
	
	@Override
	public String toString() {
		return mStrBuf.toString();
	}
	
	public static void main(String[] pArgs) {
		JsonStringBuffer jsonBuf = new JsonStringBuffer();
		
		String comment = "Device configuration contains the following fields: \n" +
				"* 'name'       : The user friendly name of the device (only used for display purposes) \n" +
				"* 'type'       : The type of the device or leds (known types for now are 'ws2801', 'lpd6803', 'sedu', 'test' and 'none') \n" +
				"* 'output'     : The output specification depends on selected device \n" +
				"                  - 'ws2801' this is the device (eg '/dev/spidev0.0 or /dev/ttyS0') \n" +
				"                 - 'test' this is the file used to write test output (eg '/home/pi/hyperion.out') \n" +
				"* 'rate'       : The baudrate of the output to the device \n" +
				"* 'colorOrder' : The order of the color bytes ('rgb', 'rbg', 'bgr', etc.). \n";
		jsonBuf.writeComment(comment);
		
		jsonBuf.startObject("device");
		jsonBuf.addValue("name", "MyPi", false);
		jsonBuf.addValue("type", "ws2801", false);
		jsonBuf.addValue("output", "/dev/spidev0.0", false);
		jsonBuf.addValue("rate", 1000000, false);
		jsonBuf.addValue("colorOrder", "rgb", true);
		jsonBuf.stopObject();
		
		jsonBuf.toggleComment(true);
		jsonBuf.startObject("device");
		jsonBuf.addValue("name", "MyPi", false);
		jsonBuf.addValue("type", "ws2801", false);
		jsonBuf.addValue("output", "/dev/spidev0.0", false);
		jsonBuf.addValue("rate", 1000000, false);
		jsonBuf.addValue("colorOrder", "rgb", true);
		jsonBuf.stopObject();
		jsonBuf.toggleComment(false);

		jsonBuf.finish();

		System.out.println(jsonBuf.toString());
	}
	
}
