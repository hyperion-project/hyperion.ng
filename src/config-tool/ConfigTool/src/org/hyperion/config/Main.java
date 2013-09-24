package org.hyperion.config;

import javax.swing.JFrame;
import javax.swing.UIManager;

import org.hyperion.config.gui.ConfigPanel;

public class Main {

	public static void main(String[] pArgs) {
		try {
			UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
		} catch (Exception e) {}
		
		JFrame frame = new JFrame();
		frame.setTitle("Hyperion configuration Tool");
		frame.setSize(1300, 600);
		frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		
		frame.setContentPane(new ConfigPanel());
		
		frame.setVisible(true);
	}
}
