package org.hyperion.hypercon;

import javax.swing.ImageIcon;
import javax.swing.JFrame;
import javax.swing.UIManager;

import org.hyperion.hypercon.gui.ConfigPanel;

/**
 * (static) Main-class for starting HyperCon (the Hyperion configuration file builder) as a standard 
 * JAVA application (contains the entry-point).
 */
public class Main {

	/**
	 * Entry point to start HyperCon 
	 * 
	 * @param pArgs HyperCon does not have command line arguments
	 */
	public static void main(String[] pArgs) {
		try {
			// Configure swing to use the system default look and feel
			UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
		} catch (Exception e) {}
		
		// Create a frame for the configuration panel
		JFrame frame = new JFrame();
		frame.setTitle("Hyperion configuration Tool");
		frame.setSize(1300, 600);
		frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		frame.setIconImage(new ImageIcon(Main.class.getResource("HyperConIcon_64.png")).getImage());
		
		// Add the HyperCon configuration panel
		frame.setContentPane(new ConfigPanel());
		
		// Show the frame
		frame.setVisible(true);
	}
}
