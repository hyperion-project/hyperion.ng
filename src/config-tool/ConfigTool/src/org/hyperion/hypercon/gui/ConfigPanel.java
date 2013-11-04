package org.hyperion.hypercon.gui;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.io.IOException;
import java.util.Observable;
import java.util.Observer;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JPanel;

import org.hyperion.hypercon.ConfigurationFile;
import org.hyperion.hypercon.LedFrameFactory;
import org.hyperion.hypercon.LedString;
import org.hyperion.hypercon.Main;

/**
 * The main-config panel of HyperCon. Includes the configuration and the panels to edit and 
 * write-out the configuration. This can be placed on JFrame, JDialog or JApplet as required.
 */
public class ConfigPanel extends JPanel {

	/** The LED configuration information*/
	private final LedString ledString;
	
	/** Action for write the Hyperion deamon configuration file */
	private final Action mSaveConfigAction = new AbstractAction("Create Hyperion Configuration") {
		JFileChooser fileChooser = new JFileChooser();
		@Override
		public void actionPerformed(ActionEvent e) {
			if (fileChooser.showSaveDialog(ConfigPanel.this) != JFileChooser.APPROVE_OPTION) {
				return;
			}

			try {
				ledString.saveConfigFile(fileChooser.getSelectedFile().getAbsolutePath());
				
				ConfigurationFile configFile = new ConfigurationFile();
				configFile.store(ledString.mDeviceConfig);
				configFile.store(ledString.mLedFrameConfig);
				configFile.store(ledString.mProcessConfig);
				configFile.store(ledString.mColorConfig);
				configFile.store(ledString.mMiscConfig);
				configFile.save(Main.configFilename);
			} catch (IOException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
		}
	};
	
	/** The panel for containing the example 'Hyperion TV' */
	private JPanel mTvPanel;
	/** The simulated 'Hyperion TV' */
	private JHyperionTv mHyperionTv;
	
	/** The left (WEST) side panel containing the diferent configuration panels */
	private JPanel mSpecificationPanel;
	
	/** The panel for specifying the construction of the LED-Frame */
	private JPanel mConstructionPanel;
	
	/** The panel for specifying the image integration */
	private JPanel mIntegrationPanel;
	
	/** The panel for specifying the miscallenuous configuration */
	private MiscConfigPanel mMiscPanel;
	
	/** The button connected to mSaveConfigAction */
	private JButton mSaveConfigButton;
	
	/**
	 * Constructs the configuration panel with a default initialised led-frame and configuration
	 */
	public ConfigPanel(final LedString pLedString) {
		super();
		
		ledString = pLedString;
		
		initialise();
		
		// Compute the individual leds for the current configuration
		ledString.leds = LedFrameFactory.construct(ledString.mLedFrameConfig, ledString.mProcessConfig);
		mHyperionTv.setLeds(ledString.leds);
		
		// Add Observer to update the individual leds if the configuration changes
		final Observer observer = new Observer() {
			@Override
			public void update(Observable o, Object arg) {
				ledString.leds = LedFrameFactory.construct(ledString.mLedFrameConfig, ledString.mProcessConfig);
				mHyperionTv.setLeds(ledString.leds);
				mHyperionTv.repaint();
			}
		};
		ledString.mLedFrameConfig.addObserver(observer);
		ledString.mProcessConfig.addObserver(observer);
	}
	
	/**
	 * Initialises the config-panel 
	 */
	private void initialise() {
		setLayout(new BorderLayout());
		
		add(getTvPanel(), BorderLayout.CENTER);
		add(getSpecificationPanel(), BorderLayout.WEST);
		
	}
	
	/**
	 * Created, if not exists, and returns the panel holding the simulated 'Hyperion TV'
	 * 
	 * @return The Tv panel
	 */
	private JPanel getTvPanel() {
		if (mTvPanel == null) {
			mTvPanel = new JPanel();
			mTvPanel.setLayout(new BorderLayout());
				
			mHyperionTv = new JHyperionTv();
			mTvPanel.add(mHyperionTv, BorderLayout.CENTER);
		}
		return mTvPanel;
	}
	
	private JPanel getSpecificationPanel() {
		if (mSpecificationPanel == null) {
			mSpecificationPanel = new JPanel();
			mSpecificationPanel.setLayout(new BoxLayout(mSpecificationPanel, BoxLayout.Y_AXIS));
			
			mConstructionPanel = new LedFramePanel(ledString.mDeviceConfig, ledString.mLedFrameConfig);
			mConstructionPanel.setBorder(BorderFactory.createTitledBorder("Construction"));
			mSpecificationPanel.add(mConstructionPanel);

			mIntegrationPanel = new ImageProcessPanel(ledString.mProcessConfig);
			mIntegrationPanel.setBorder(BorderFactory.createTitledBorder("Image Process"));
			mSpecificationPanel.add(mIntegrationPanel);
			
			mMiscPanel = new MiscConfigPanel(ledString.mMiscConfig);
			mMiscPanel.setBorder(BorderFactory.createTitledBorder("Misc"));
			mSpecificationPanel.add(mMiscPanel);
			
			JPanel panel = new JPanel(new BorderLayout());
			panel.setBorder(BorderFactory.createEmptyBorder(5,5,5,5));
			mSaveConfigButton = new JButton(mSaveConfigAction);
			panel.add(mSaveConfigButton, BorderLayout.SOUTH);
			mSpecificationPanel.add(panel);
		}
		return mSpecificationPanel;
	}
}
