package org.hyperion.config;

import java.awt.BorderLayout;
import java.awt.Dimension;

import javax.swing.JApplet;

import org.hyperion.config.gui.ConfigPanel;

public class HyperionConfigApplet extends JApplet {

	public HyperionConfigApplet() {
		super();
		
		initialise();
	}
	
	private void initialise() {
		setPreferredSize(new Dimension(600, 300));
		
		add(new ConfigPanel(), BorderLayout.CENTER);
	}
}
