package org.hyperion.hypercon.gui;

import javax.swing.BorderFactory;
import javax.swing.GroupLayout;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

public class EffectEnginePanel extends JPanel {

	private JLabel mPathLabel;
	private JTextField mPathField;
	
	private JPanel mBootSequencePanel;
	private JCheckBox mBootSequenceCheck;
	private JLabel mBootSequenceLabel;
	private JTextField mBootSequenceField;
	
	public EffectEnginePanel() {
		super();
		
		initialise();
	}
	
	private void initialise() {
		setBorder(BorderFactory.createTitledBorder("Effect Engine"));
		
		mPathLabel = new JLabel("Directory: ");
		add(mPathLabel);
		
		mPathField = new JTextField();
		add(mPathField);
		
		add(getBootSequencePanel());
		
		GroupLayout layout = new GroupLayout(this);
		setLayout(layout);
		
		layout.setVerticalGroup(layout.createSequentialGroup()
				.addGroup(layout.createParallelGroup()
						.addComponent(mPathLabel)
						.addComponent(mPathField))
				.addComponent(getBootSequencePanel()));
		
		layout.setHorizontalGroup(layout.createParallelGroup()
				.addGroup(layout.createSequentialGroup()
						.addComponent(mPathLabel)
						.addComponent(mPathField))
				.addComponent(getBootSequencePanel()));
	}
	
	private JPanel getBootSequencePanel() {
		if (mBootSequencePanel == null) {
			mBootSequencePanel = new JPanel();
			mBootSequencePanel.setBorder(BorderFactory.createTitledBorder("Bootsequence"));
			
			mBootSequenceCheck = new JCheckBox("Enabled");
			mBootSequencePanel.add(mBootSequenceCheck);
			
			mBootSequenceLabel = new JLabel("Type:");
			mBootSequencePanel.add(mBootSequenceLabel);
			
			mBootSequenceField = new JTextField();
			mBootSequencePanel.add(mBootSequenceField);
			
			GroupLayout layout = new GroupLayout(mBootSequencePanel);
			mBootSequencePanel.setLayout(layout);
			
			layout.setVerticalGroup(layout.createSequentialGroup()
					.addGroup(layout.createParallelGroup()
							.addComponent(mBootSequenceLabel)
							.addComponent(mBootSequenceField))
					.addComponent(mBootSequenceCheck));
			
			layout.setHorizontalGroup(layout.createParallelGroup()
					.addGroup(layout.createSequentialGroup()
							.addComponent(mBootSequenceLabel)
							.addComponent(mBootSequenceField))
					.addComponent(mBootSequenceCheck));
			
		}
		return mBootSequencePanel;
	}
}
