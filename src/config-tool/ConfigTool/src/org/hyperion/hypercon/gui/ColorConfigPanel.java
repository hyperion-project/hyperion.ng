package org.hyperion.hypercon.gui;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.GroupLayout;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import org.hyperion.hypercon.spec.ColorConfig;

/**
 * Configuration panel for the ColorConfig.
 *  
 * NB This has not been integrated in the GUI jet!
 */
public class ColorConfigPanel extends JPanel {
	
	private final ColorConfig mColorConfig;
	
	private JPanel mRgbTransformPanel;
	private JLabel mThresholdLabel;
	private JLabel mGammaLabel;
	private JLabel mBlacklevelLabel;
	private JLabel mWhitelevelLabel;
	private JLabel mRedTransformLabel;
	private JSpinner mRedThresholdSpinner;
	private JSpinner mRedGammaSpinner;
	private JSpinner mRedBlacklevelSpinner;
	private JSpinner mRedWhitelevelSpinner;
	private JLabel mGreenTransformLabel;
	private JSpinner mGreenThresholdSpinner;
	private JSpinner mGreenGammaSpinner;
	private JSpinner mGreenBlacklevelSpinner;
	private JSpinner mGreenWhitelevelSpinner;
	private JLabel mBlueTransformLabel;
	private JSpinner mBlueThresholdSpinner;
	private JSpinner mBlueGammaSpinner;
	private JSpinner mBlueBlacklevelSpinner;
	private JSpinner mBlueWhitelevelSpinner;

	private JPanel mHsvTransformPanel;
	private JLabel mSaturationAdjustLabel;
	private JSpinner mSaturationAdjustSpinner;
	private JLabel mValueAdjustLabel;
	private JSpinner mValueAdjustSpinner;

	public ColorConfigPanel(ColorConfig pColorConfig) {
		super();
		
		mColorConfig = pColorConfig;
		
		initialise();
	}
	
	private void initialise() {
		setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
		
		add(getRgbPanel());
		add(getHsvPanel());
	}
	
	private JPanel getRgbPanel() {
		if (mRgbTransformPanel == null) {
			mRgbTransformPanel = new JPanel();
			
			GroupLayout layout = new GroupLayout(mRgbTransformPanel);
			mRgbTransformPanel.setLayout(layout);
			
			mThresholdLabel = new JLabel("Thresold");
			mRgbTransformPanel.add(mThresholdLabel);
			
			mGammaLabel = new JLabel("Gamma");
			mRgbTransformPanel.add(mGammaLabel);
			
			mBlacklevelLabel = new JLabel("Blacklevel");
			mRgbTransformPanel.add(mBlacklevelLabel);
			
			mWhitelevelLabel = new JLabel("Whitelevel");
			mRgbTransformPanel.add(mWhitelevelLabel);
			
			mRedTransformLabel = new JLabel("RED");
			mRgbTransformPanel.add(mRedTransformLabel);
			mRedThresholdSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mRedThreshold, 0.0, 1.0, 0.1));
			mRedThresholdSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mRedThresholdSpinner);
			mRedGammaSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mRedGamma, 0.0, 100.0, 0.1));
			mRedThresholdSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mRedGammaSpinner);
			mRedBlacklevelSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mRedBlacklevel, 0.0, 1.0, 0.1));
			mRedThresholdSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mRedBlacklevelSpinner);
			mRedWhitelevelSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mRedWhitelevel, 0.0, 1.0, 0.1));
			mRedThresholdSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mRedWhitelevelSpinner);
			
			mGreenTransformLabel = new JLabel("GREEN");
			mRgbTransformPanel.add(mGreenTransformLabel);
			mGreenThresholdSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mGreenThreshold, 0.0, 1.0, 0.1));
			mGreenThresholdSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mGreenThresholdSpinner);
			mGreenGammaSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mGreenGamma, 0.0, 100.0, 0.1));
			mGreenGammaSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mGreenGammaSpinner);
			mGreenBlacklevelSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mGreenBlacklevel, 0.0, 1.0, 0.1));
			mGreenBlacklevelSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mGreenBlacklevelSpinner);
			mGreenWhitelevelSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mGreenWhitelevel, 0.0, 1.0, 0.1));
			mGreenWhitelevelSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mGreenWhitelevelSpinner);

			mBlueTransformLabel = new JLabel("BLUE");
			mRgbTransformPanel.add(mBlueTransformLabel);
			mBlueThresholdSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mBlueThreshold, 0.0, 1.0, 0.1));
			mBlueThresholdSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mBlueThresholdSpinner);
			mBlueGammaSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mBlueGamma, 0.0, 100.0, 0.1));
			mBlueGammaSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mBlueGammaSpinner);
			mBlueBlacklevelSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mBlueBlacklevel, 0.0, 1.0, 0.1));
			mBlueBlacklevelSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mBlueBlacklevelSpinner);
			mBlueWhitelevelSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mBlueWhitelevel, 0.0, 1.0, 0.1));
			mBlueWhitelevelSpinner.addChangeListener(mChangeListener);
			mRgbTransformPanel.add(mBlueWhitelevelSpinner);
			
			layout.setHorizontalGroup(layout.createSequentialGroup()
					.addGroup(layout.createParallelGroup()
							.addComponent(mRedTransformLabel)
							.addComponent(mGreenTransformLabel)
							.addComponent(mBlueTransformLabel))
					.addGroup(layout.createParallelGroup()
							.addComponent(mThresholdLabel)
							.addComponent(mRedThresholdSpinner)
							.addComponent(mGreenThresholdSpinner)
							.addComponent(mBlueThresholdSpinner))
					.addGroup(layout.createParallelGroup()
							.addComponent(mGammaLabel)
							.addComponent(mRedGammaSpinner)
							.addComponent(mGreenGammaSpinner)
							.addComponent(mBlueGammaSpinner)
							)
					.addGroup(layout.createParallelGroup()
							.addComponent(mBlacklevelLabel)
							.addComponent(mRedBlacklevelSpinner)
							.addComponent(mGreenBlacklevelSpinner)
							.addComponent(mBlueBlacklevelSpinner)
							)
					.addGroup(layout.createParallelGroup()
							.addComponent(mWhitelevelLabel)
							.addComponent(mRedWhitelevelSpinner)
							.addComponent(mGreenWhitelevelSpinner)
							.addComponent(mBlueWhitelevelSpinner)
							));
			
			layout.setVerticalGroup(layout.createSequentialGroup()
					.addGroup(layout.createParallelGroup()
							.addComponent(mThresholdLabel)
							.addComponent(mGammaLabel)
							.addComponent(mBlacklevelLabel)
							.addComponent(mWhitelevelLabel))
					.addGroup(layout.createParallelGroup()
							.addComponent(mRedTransformLabel)
							.addComponent(mRedThresholdSpinner)
							.addComponent(mRedGammaSpinner)
							.addComponent(mRedBlacklevelSpinner)
							.addComponent(mRedWhitelevelSpinner)
							)
					.addGroup(layout.createParallelGroup()
							.addComponent(mGreenTransformLabel)
							.addComponent(mGreenThresholdSpinner)
							.addComponent(mGreenGammaSpinner)
							.addComponent(mGreenBlacklevelSpinner)
							.addComponent(mGreenWhitelevelSpinner)
							)
					.addGroup(layout.createParallelGroup()
							.addComponent(mBlueTransformLabel)
							.addComponent(mBlueThresholdSpinner)
							.addComponent(mBlueGammaSpinner)
							.addComponent(mBlueBlacklevelSpinner)
							.addComponent(mBlueWhitelevelSpinner)
							));
		}
		return mRgbTransformPanel;
	}
	
	private JPanel getHsvPanel() {
		if (mHsvTransformPanel == null) {
			mHsvTransformPanel = new JPanel();
			mHsvTransformPanel.setBorder(BorderFactory.createTitledBorder("HSV"));
			
			GroupLayout layout = new GroupLayout(mHsvTransformPanel);
			mHsvTransformPanel.setLayout(layout);
			
			mSaturationAdjustLabel = new JLabel("Saturation");
			mHsvTransformPanel.add(mSaturationAdjustLabel);
			
			mSaturationAdjustSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mSaturationGain, 0.0, 1024.0, 0.01));
			mSaturationAdjustSpinner.addChangeListener(mChangeListener);
			mHsvTransformPanel.add(mSaturationAdjustSpinner);
			
			mValueAdjustLabel = new JLabel("Value");
			mHsvTransformPanel.add(mValueAdjustLabel);
			
			mValueAdjustSpinner = new JSpinner(new SpinnerNumberModel(mColorConfig.mValueGain, 0.0, 1024.0, 0.01));
			mValueAdjustSpinner.addChangeListener(mChangeListener);
			mHsvTransformPanel.add(mValueAdjustSpinner);

			layout.setHorizontalGroup(layout.createSequentialGroup()
					.addGroup(layout.createParallelGroup()
							.addComponent(mSaturationAdjustLabel)
							.addComponent(mValueAdjustLabel)
							)
					.addGroup(layout.createParallelGroup()
							.addComponent(mSaturationAdjustSpinner)
							.addComponent(mValueAdjustSpinner)
							)
					);
			
			layout.setVerticalGroup(layout.createSequentialGroup()
					.addGroup(layout.createParallelGroup()
							.addComponent(mSaturationAdjustLabel)
							.addComponent(mSaturationAdjustSpinner)
							)
					.addGroup(layout.createParallelGroup()
							.addComponent(mValueAdjustLabel)
							.addComponent(mValueAdjustSpinner)
							)
					);
		}
		return mHsvTransformPanel;
	}
	
	private final ChangeListener mChangeListener = new ChangeListener() {
		@Override
		public void stateChanged(ChangeEvent e) {
			mColorConfig.mRedThreshold  = (Double)mRedThresholdSpinner.getValue();
			mColorConfig.mRedGamma      = (Double)mRedGammaSpinner.getValue();
			mColorConfig.mRedBlacklevel = (Double)mRedBlacklevelSpinner.getValue();
			mColorConfig.mRedWhitelevel = (Double)mRedWhitelevelSpinner.getValue();

			mColorConfig.mGreenThreshold  = (Double)mGreenThresholdSpinner.getValue();
			mColorConfig.mGreenGamma      = (Double)mGreenGammaSpinner.getValue();
			mColorConfig.mGreenBlacklevel = (Double)mGreenBlacklevelSpinner.getValue();
			mColorConfig.mGreenWhitelevel = (Double)mGreenWhitelevelSpinner.getValue();

			mColorConfig.mBlueThreshold  = (Double)mBlueThresholdSpinner.getValue();
			mColorConfig.mBlueGamma      = (Double)mBlueGammaSpinner.getValue();
			mColorConfig.mBlueBlacklevel = (Double)mBlueBlacklevelSpinner.getValue();
			mColorConfig.mBlueWhitelevel = (Double)mBlueWhitelevelSpinner.getValue();
			
			mColorConfig.mSaturationGain = (Double)mSaturationAdjustSpinner.getValue();
			mColorConfig.mValueGain      = (Double)mValueAdjustSpinner.getValue();
		}
	};
}
