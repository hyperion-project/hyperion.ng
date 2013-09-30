package org.hyperion.hypercon;

import java.awt.BorderLayout;
import java.awt.Dimension;

import javax.swing.JApplet;

import org.hyperion.hypercon.gui.ConfigPanel;

/**
 * Class for starting HyperCon (Hyperion configuration file builder) as a Applet (within a browser)
 *
 */
public class HyperConApplet extends JApplet {

	/**
	 * Constructs the HyperCon Applet
	 */
	public HyperConApplet() {
		super();
		
		initialise();
	}
	
	/**
	 * Initialises this applet
	 */
	private void initialise() {
		setPreferredSize(new Dimension(600, 300));
		
		// Add the HyperCon configuration panel
		add(new ConfigPanel(), BorderLayout.CENTER);
	}
}
