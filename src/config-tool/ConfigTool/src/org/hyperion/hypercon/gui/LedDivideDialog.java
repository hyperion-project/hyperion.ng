package org.hyperion.hypercon.gui;

import java.awt.GridLayout;

import javax.swing.ButtonGroup;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JRadioButton;

public class LedDivideDialog extends JFrame {

	private final int mLedCount;
	private final int mTransformCount;
	
	private JPanel mContentPanel;
	
	public LedDivideDialog(int pLedCnt, int pTransformCnt) {
		super();
		
		mLedCount = pLedCnt;
		mTransformCount = pTransformCnt;
		
		initialise();
	}
	
	private void initialise() {
		mContentPanel = new JPanel();
		mContentPanel.setLayout(new GridLayout(mLedCount, mTransformCount, 5, 5));
		
		for (int iLed=0; iLed<mLedCount; ++iLed) {
			ButtonGroup ledGroup = new ButtonGroup();
			for (int iTransform=0; iTransform<mTransformCount; ++iTransform) {
				JRadioButton ledTransformButton = new JRadioButton();
				ledGroup.add(ledTransformButton);
				mContentPanel.add(ledTransformButton);
			}
		}
		
		setContentPane(mContentPanel);
	}
	
	
	public static void main(String[] pArgs) {
		LedDivideDialog dialog = new LedDivideDialog(50, 3);
		dialog.setSize(600, 800);
		dialog.setVisible(true);
	}
}
