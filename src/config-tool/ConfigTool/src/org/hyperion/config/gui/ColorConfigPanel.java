package org.hyperion.config.gui;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.GroupLayout;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;

public class ColorConfigPanel extends JPanel {
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

	public ColorConfigPanel() {
		super();
		
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
			mRedThresholdSpinner = new JSpinner(new SpinnerNumberModel());
			mRgbTransformPanel.add(mRedThresholdSpinner);
			mRedGammaSpinner = new JSpinner(new SpinnerNumberModel());
			mRgbTransformPanel.add(mRedGammaSpinner);
			mRedBlacklevelSpinner = new JSpinner(new SpinnerNumberModel());
			mRgbTransformPanel.add(mRedBlacklevelSpinner);
			mRedWhitelevelSpinner = new JSpinner(new SpinnerNumberModel());
			mRgbTransformPanel.add(mRedWhitelevelSpinner);
			
			mGreenTransformLabel = new JLabel("GREEN");
			mRgbTransformPanel.add(mGreenTransformLabel);
			mGreenThresholdSpinner = new JSpinner(new SpinnerNumberModel());
			mRgbTransformPanel.add(mGreenThresholdSpinner);
			mGreenGammaSpinner = new JSpinner(new SpinnerNumberModel());
			mRgbTransformPanel.add(mGreenGammaSpinner);
			mGreenBlacklevelSpinner = new JSpinner(new SpinnerNumberModel());
			mRgbTransformPanel.add(mGreenBlacklevelSpinner);
			mGreenWhitelevelSpinner = new JSpinner(new SpinnerNumberModel());
			mRgbTransformPanel.add(mGreenWhitelevelSpinner);

			mBlueTransformLabel = new JLabel("BLUE");
			mRgbTransformPanel.add(mBlueTransformLabel);
			mBlueThresholdSpinner = new JSpinner(new SpinnerNumberModel());
			mRgbTransformPanel.add(mBlueThresholdSpinner);
			mBlueGammaSpinner = new JSpinner(new SpinnerNumberModel());
			mRgbTransformPanel.add(mBlueGammaSpinner);
			mBlueBlacklevelSpinner = new JSpinner(new SpinnerNumberModel());
			mRgbTransformPanel.add(mBlueBlacklevelSpinner);
			mBlueWhitelevelSpinner = new JSpinner(new SpinnerNumberModel());
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
			
			mSaturationAdjustSpinner = new JSpinner(new SpinnerNumberModel(1.0, 0.0, 1024.0, 0.01));
			mHsvTransformPanel.add(mSaturationAdjustSpinner);
			
			mValueAdjustLabel = new JLabel("Value");
			mHsvTransformPanel.add(mValueAdjustLabel);
			
			mValueAdjustSpinner = new JSpinner(new SpinnerNumberModel(1.0, 0.0, 1024.0, 0.01));
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
	
}
