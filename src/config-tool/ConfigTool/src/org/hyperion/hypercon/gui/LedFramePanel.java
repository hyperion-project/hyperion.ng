package org.hyperion.hypercon.gui;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.beans.Transient;

import javax.swing.BorderFactory;
import javax.swing.GroupLayout;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import org.hyperion.hypercon.spec.LedFrameConstruction;
import org.hyperion.hypercon.spec.LedFrameConstruction.Direction;

public class LedFramePanel extends JPanel {
	
	private final LedFrameConstruction mLedFrameSpec;

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
	
	@Override
	@Transient
	public Dimension getMaximumSize() {
		Dimension maxSize = super.getMaximumSize();
		Dimension prefSize = super.getPreferredSize();
		return new Dimension(maxSize.width, prefSize.height);
	}

	private void initialise() {
		setBorder(BorderFactory.createTitledBorder("Construction"));
		
		mTopCornerLabel = new JLabel("Led in top corners");
		add(mTopCornerLabel);
		mTopCornerCombo = new JComboBox<>(new Boolean[] {true, false});
		mTopCornerCombo.setSelectedItem(mLedFrameSpec.topCorners);
		mTopCornerCombo.addActionListener(mActionListener);
		add(mTopCornerCombo);
		
		mBottomCornerLabel = new JLabel("Led in bottom corners");
		add(mBottomCornerLabel);
		mBottomCornerCombo = new JComboBox<>(new Boolean[] {true, false});
		mBottomCornerCombo.setSelectedItem(mLedFrameSpec.bottomCorners);
		mBottomCornerCombo.addActionListener(mActionListener);
		add(mBottomCornerCombo);
		
		mDirectionLabel = new JLabel("Direction");
		add(mDirectionLabel);
		mDirectionCombo = new JComboBox<>(LedFrameConstruction.Direction.values());
		mDirectionCombo.setSelectedItem(mLedFrameSpec.clockwiseDirection?Direction.clockwise:Direction.counter_clockwise);
		mDirectionCombo.addActionListener(mActionListener);
		add(mDirectionCombo);

		mHorizontalCountLabel = new JLabel("Horizontal #:");
		add(mHorizontalCountLabel);
		mHorizontalCountSpinner = new JSpinner(new SpinnerNumberModel(mLedFrameSpec.topLedCnt, 0, 1024, 1));
		mHorizontalCountSpinner.addChangeListener(mChangeListener);
		add(mHorizontalCountSpinner);
		
		mBottomGapCountLabel = new JLabel("Bottom Gap #:");
		add(mBottomGapCountLabel);
		mBottomGapCountSpinner = new JSpinner(new SpinnerNumberModel(mLedFrameSpec.topLedCnt - mLedFrameSpec.bottomLedCnt, 0, 1024, 1));
		mBottomGapCountSpinner.addChangeListener(mChangeListener);
		add(mBottomGapCountSpinner);

		mVerticalCountLabel = new JLabel("Vertical #:");
		add(mVerticalCountLabel);
		mVerticalCountSpinner = new JSpinner(new SpinnerNumberModel(mLedFrameSpec.rightLedCnt, 0, 1024, 1));
		mVerticalCountSpinner.addChangeListener(mChangeListener);
		add(mVerticalCountSpinner);

		mOffsetLabel = new JLabel("1st LED offset");
		add(mOffsetLabel);
		mOffsetSpinner = new JSpinner(new SpinnerNumberModel(mLedFrameSpec.firstLedOffset, Integer.MIN_VALUE, Integer.MAX_VALUE, 1));
		mOffsetSpinner.addChangeListener(mChangeListener);
		add(mOffsetSpinner);

		GroupLayout layout = new GroupLayout(this);
		layout.setAutoCreateGaps(true);
		setLayout(layout);
		
		layout.setHorizontalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mDirectionLabel)
						.addComponent(mTopCornerLabel)
						.addComponent(mBottomCornerLabel)
						.addComponent(mHorizontalCountLabel)
						.addComponent(mBottomGapCountLabel)
						.addComponent(mVerticalCountLabel)
						.addComponent(mOffsetLabel))
				.addGroup(layout.createParallelGroup()
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
		mLedFrameSpec.topCorners    = (Boolean)mTopCornerCombo.getSelectedItem();
		mLedFrameSpec.bottomCorners = (Boolean)mBottomCornerCombo.getSelectedItem();
		
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
