package org.hyperion.config.gui;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.GroupLayout;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import org.hyperion.config.spec.DeviceType;
import org.hyperion.config.spec.LedFrameConstruction;

public class LedFramePanel extends JPanel {
	
	private final LedFrameConstruction mLedFrameSpec;
	
	private JLabel mTypeLabel;
	private JComboBox<DeviceType> mTypeCombo;

	private JLabel mHorizontalCountLabel;
	private JSpinner mHorizontalCountSpinner;
	private JLabel mBottomGapCountLabel;
	private JSpinner mBottomGapCountSpinner;
	
	private JLabel mVerticalCountLabel;
	private JSpinner mVerticalCountSpinner;

	private JLabel mTopCornerLabel;
	private JComboBox<Boolean> mTopCornerCombo;
	private JLabel mBottomCornerLabel;
	private JComboBox<Boolean> mBottomCornerCombo;
	
	private JLabel mDirectionLabel;
	private JComboBox<LedFrameConstruction.Direction> mDirectionCombo;
	
	private JLabel mOffsetLabel;
	private JSpinner mOffsetSpinner;
	
	public LedFramePanel(LedFrameConstruction ledFrameSpec) {
		super();
		
		mLedFrameSpec = ledFrameSpec;
		
		initialise();
	}
	
	private void initialise() {
		mTypeLabel = new JLabel("LED Type:");
		mTypeLabel.setPreferredSize(new Dimension(100, 30));
		mTypeLabel.setMaximumSize(new Dimension(150, 30));
		add(mTypeLabel);
		mTypeCombo = new JComboBox<>(DeviceType.values());
		mTypeCombo.addActionListener(mActionListener);
		mTypeCombo.setPreferredSize(new Dimension(150, 30));
		mTypeCombo.setMaximumSize(new Dimension(250, 30));
		add(mTypeCombo);
		
		mTopCornerLabel = new JLabel("Led in top corners");
		mTopCornerLabel.setPreferredSize(new Dimension(100, 30));
		mTopCornerLabel.setMaximumSize(new Dimension(150, 30));
		add(mTopCornerLabel);
		mTopCornerCombo = new JComboBox<>(new Boolean[] {true, false});
		mTopCornerCombo.addActionListener(mActionListener);
		mTopCornerCombo.setPreferredSize(new Dimension(150, 30));
		mTopCornerCombo.setMaximumSize(new Dimension(250, 30));
		add(mTopCornerCombo);
		
		mBottomCornerLabel = new JLabel("Led in bottom corners");
		mBottomCornerLabel.setPreferredSize(new Dimension(100, 30));
		mBottomCornerLabel.setMaximumSize(new Dimension(150, 30));
		add(mBottomCornerLabel);
		mBottomCornerCombo = new JComboBox<>(new Boolean[] {true, false});
		mBottomCornerCombo.addActionListener(mActionListener);
		mBottomCornerCombo.setPreferredSize(new Dimension(150, 30));
		mBottomCornerCombo.setMaximumSize(new Dimension(250, 30));
		add(mBottomCornerCombo);
		
		mDirectionLabel = new JLabel("Direction");
		mDirectionLabel.setPreferredSize(new Dimension(100, 30));
		mDirectionLabel.setMaximumSize(new Dimension(150, 30));
		add(mDirectionLabel);
		mDirectionCombo = new JComboBox<>(LedFrameConstruction.Direction.values());
		mDirectionCombo.addActionListener(mActionListener);
		mDirectionCombo.setPreferredSize(new Dimension(150, 30));
		mDirectionCombo.setMaximumSize(new Dimension(250, 30));
		add(mDirectionCombo);

		mHorizontalCountLabel = new JLabel("Horizontal #:");
		mHorizontalCountLabel.setPreferredSize(new Dimension(100, 30));
		mHorizontalCountLabel.setMaximumSize(new Dimension(150, 30));
		add(mHorizontalCountLabel);
		mHorizontalCountSpinner = new JSpinner(new SpinnerNumberModel(mLedFrameSpec.topLedCnt, 0, 1024, 1));
		mHorizontalCountSpinner.addChangeListener(mChangeListener);
		mHorizontalCountSpinner.setPreferredSize(new Dimension(150, 30));
		mHorizontalCountSpinner.setMaximumSize(new Dimension(250, 30));
		add(mHorizontalCountSpinner);
		
		mBottomGapCountLabel = new JLabel("Bottom Gap #:");
		mBottomGapCountLabel.setPreferredSize(new Dimension(100, 30));
		mBottomGapCountLabel.setMaximumSize(new Dimension(150, 30));
		add(mBottomGapCountLabel);
		mBottomGapCountSpinner = new JSpinner(new SpinnerNumberModel(mLedFrameSpec.topLedCnt - mLedFrameSpec.bottomLedCnt, 0, 1024, 1));
		mBottomGapCountSpinner.addChangeListener(mChangeListener);
		mBottomGapCountSpinner.setPreferredSize(new Dimension(150, 30));
		mBottomGapCountSpinner.setMaximumSize(new Dimension(250, 30));
		add(mBottomGapCountSpinner);

		mVerticalCountLabel = new JLabel("Vertical #:");
		mVerticalCountLabel.setPreferredSize(new Dimension(100, 30));
		mVerticalCountLabel.setMaximumSize(new Dimension(150, 30));
		add(mVerticalCountLabel);
		mVerticalCountSpinner = new JSpinner(new SpinnerNumberModel(mLedFrameSpec.rightLedCnt, 0, 1024, 1));
		mVerticalCountSpinner.addChangeListener(mChangeListener);
		mVerticalCountSpinner.setPreferredSize(new Dimension(150, 30));
		mVerticalCountSpinner.setMaximumSize(new Dimension(250, 30));
		add(mVerticalCountSpinner);

		mOffsetLabel = new JLabel("1st LED offset");
		mOffsetLabel.setPreferredSize(new Dimension(100, 30));
		mOffsetLabel.setMaximumSize(new Dimension(150, 30));
		add(mOffsetLabel);
		mOffsetSpinner = new JSpinner(new SpinnerNumberModel(mLedFrameSpec.firstLedOffset, Integer.MIN_VALUE, Integer.MAX_VALUE, 1));
		mOffsetSpinner.addChangeListener(mChangeListener);
		mOffsetSpinner.setPreferredSize(new Dimension(150, 30));
		mOffsetSpinner.setMaximumSize(new Dimension(250, 30));
		add(mOffsetSpinner);

		GroupLayout layout = new GroupLayout(this);
		layout.setAutoCreateGaps(true);
		setLayout(layout);
		
		layout.setHorizontalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mTypeLabel)
						.addComponent(mDirectionLabel)
						.addComponent(mTopCornerLabel)
						.addComponent(mBottomCornerLabel)
						.addComponent(mHorizontalCountLabel)
						.addComponent(mBottomGapCountLabel)
						.addComponent(mVerticalCountLabel)
						.addComponent(mOffsetLabel))
				.addGroup(layout.createParallelGroup()
						.addComponent(mTypeCombo)
						.addComponent(mDirectionCombo)
						.addComponent(mTopCornerCombo)
						.addComponent(mBottomCornerCombo)
						.addComponent(mHorizontalCountSpinner)
						.addComponent(mBottomGapCountSpinner)
						.addComponent(mVerticalCountSpinner)
						.addComponent(mOffsetSpinner))
				);
		layout.setVerticalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mTypeLabel)
						.addComponent(mTypeCombo))
				.addGroup(layout.createParallelGroup()
						.addComponent(mDirectionLabel)
						.addComponent(mDirectionCombo))
				.addGroup(layout.createParallelGroup()
						.addComponent(mTopCornerLabel)
						.addComponent(mTopCornerCombo))
				.addGroup(layout.createParallelGroup()
						.addComponent(mBottomCornerLabel)
						.addComponent(mBottomCornerCombo))
				.addGroup(layout.createParallelGroup()
						.addComponent(mHorizontalCountLabel)
						.addComponent(mHorizontalCountSpinner))
				.addGroup(layout.createParallelGroup()
						.addComponent(mVerticalCountLabel)
						.addComponent(mVerticalCountSpinner))
				.addGroup(layout.createParallelGroup()
						.addComponent(mBottomGapCountLabel)
						.addComponent(mBottomGapCountSpinner))
				.addGroup(layout.createParallelGroup()
						.addComponent(mOffsetLabel)
						.addComponent(mOffsetSpinner)));
		
	}
	
	void updateLedConstruction() {
		mLedFrameSpec.topLeftCorner  = (Boolean)mTopCornerCombo.getSelectedItem();
		mLedFrameSpec.topRightCorner = (Boolean)mTopCornerCombo.getSelectedItem();
		mLedFrameSpec.bottomLeftCorner  = (Boolean)mBottomCornerCombo.getSelectedItem();
		mLedFrameSpec.bottomRightCorner = (Boolean)mBottomCornerCombo.getSelectedItem();
		
		mLedFrameSpec.clockwiseDirection = ((LedFrameConstruction.Direction)mDirectionCombo.getSelectedItem()) == LedFrameConstruction.Direction.clockwise;
		mLedFrameSpec.firstLedOffset = (Integer)mOffsetSpinner.getValue();
		
		mLedFrameSpec.topLedCnt = (Integer)mHorizontalCountSpinner.getValue();
		mLedFrameSpec.bottomLedCnt = Math.max(0, mLedFrameSpec.topLedCnt - (Integer)mBottomGapCountSpinner.getValue());
		mLedFrameSpec.rightLedCnt = (Integer)mVerticalCountSpinner.getValue();
		mLedFrameSpec.leftLedCnt  = (Integer)mVerticalCountSpinner.getValue();
		
		mLedFrameSpec.setChanged();
		mLedFrameSpec.notifyObservers();
		
		mBottomGapCountSpinner.setValue(mLedFrameSpec.topLedCnt - mLedFrameSpec.bottomLedCnt);
	}
	
	private final ActionListener mActionListener = new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
			updateLedConstruction();
		}
	};
	private final ChangeListener mChangeListener = new ChangeListener() {
		@Override
		public void stateChanged(ChangeEvent e) {
			updateLedConstruction();
		}
	};

}
