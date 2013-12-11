package org.hyperion.hypercon;

import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;

import javax.swing.ImageIcon;
import javax.swing.JFrame;
import javax.swing.UIManager;

import org.hyperion.hypercon.gui.ConfigPanel;

/**
 * (static) Main-class for starting HyperCon (the Hyperion configuration file builder) as a standard 
 * JAVA application (contains the entry-point).
 */
public class Main {
	public static final String configFilename = "hypercon.dat";
	
	/** Some application settings (for easy/dirty access) */
	public static final HyperConConfig HyperConConfig = new HyperConConfig();

	/**
	 * Entry point to start HyperCon 
	 * 
	 * @param pArgs HyperCon does not have command line arguments
	 */
	public static void main(String[] pArgs) {
		final String versionStr = Main.class.getPackage().getImplementationVersion();
		final LedString ledString = new LedString();
		
		try {
			// Configure swing to use the system default look and feel
			UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
		} catch (Exception e) {}
		
		// Create a frame for the configuration panel
		JFrame frame = new JFrame();
		String title = "Hyperion configuration Tool" + ((versionStr != null && !versionStr.isEmpty())? (" (" + versionStr + ")") : ""); 
		frame.setTitle(title);
		frame.setSize(1300, 700);
		frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		frame.setIconImage(new ImageIcon(Main.class.getResource("HyperConIcon_64.png")).getImage());
		frame.addWindowListener(new WindowAdapter() {
			@Override
			public void windowClosing(WindowEvent e) {
				ConfigurationFile configFile = new ConfigurationFile();
				configFile.store(Main.HyperConConfig);
				configFile.store(ledString.mDeviceConfig);
				configFile.store(ledString.mLedFrameConfig);
				configFile.store(ledString.mProcessConfig);
				configFile.store(ledString.mColorConfig);
				configFile.store(ledString.mMiscConfig);
				configFile.store(ledString.mEffectEngineConfig);
				configFile.save(configFilename);
			}
		});
		
		if (new File(configFilename).exists()) {
			ConfigurationFile configFile = new ConfigurationFile();
			configFile.load(configFilename);
			configFile.restore(Main.HyperConConfig);
			configFile.restore(ledString.mDeviceConfig);
			configFile.restore(ledString.mLedFrameConfig);
			configFile.restore(ledString.mProcessConfig);
			configFile.restore(ledString.mColorConfig);
			configFile.restore(ledString.mMiscConfig);
			configFile.restore(ledString.mEffectEngineConfig);
			
			if (HyperConConfig.loadDefaultEffect) {
				ledString.mEffectEngineConfig.loadDefault();
				HyperConConfig.loadDefaultEffect = false;
			}
		}
		
		// Add the HyperCon configuration panel
		frame.setContentPane(new ConfigPanel(ledString));
		
		// Show the frame
		frame.setVisible(true);
	}
}
