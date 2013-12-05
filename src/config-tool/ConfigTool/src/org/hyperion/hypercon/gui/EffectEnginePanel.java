package org.hyperion.hypercon.gui;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.DefaultComboBoxModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextArea;

import org.hyperion.hypercon.spec.EffectConfig;
import org.hyperion.hypercon.spec.EffectEngineConfig;

public class EffectEnginePanel extends JPanel {

	private final EffectEngineConfig mEffectEngingeConfig;
	
	private final DefaultComboBoxModel<EffectConfig> mEffectModel;
	
	private JPanel mControlPanel;
	private JComboBox<EffectConfig> mEffectCombo;
	private JButton mAddButton;
	private JButton mDelButton;
	
	private JPanel mEffectPanel;
	private JLabel mPythonLabel;
	private JComboBox<String> mPythonCombo;
	private JLabel mJsonArgumentLabel;
	private JTextArea mJsonArgumentArea;
	
	public EffectEnginePanel(final EffectEngineConfig pEffectEngineConfig) {
		super();
		
		mEffectEngingeConfig = pEffectEngineConfig;
		mEffectModel = new DefaultComboBoxModel<EffectConfig>(mEffectEngingeConfig.mEffects);
		
		initialise();
		
		effectSelectionChanged();
	}
	
	private void initialise() {
		setLayout(new BorderLayout());
		
		add(getControlPanel(), BorderLayout.NORTH);
		add(getEffectPanel(), BorderLayout.CENTER);
	}
	
	private void effectSelectionChanged() {
		EffectConfig effect = (EffectConfig)mEffectModel.getSelectedItem();
		
		// Enable option for the selected effect or disable if none selected
		mEffectPanel.setEnabled(effect != null);
		mPythonLabel.setEnabled(effect != null);
		mPythonCombo.setEnabled(effect != null);
		mJsonArgumentLabel.setEnabled(effect != null);
		mJsonArgumentArea.setEnabled(effect != null);
		
		
		if (effect == null) {
			// Clear all fields
			mEffectPanel.setBorder(BorderFactory.createTitledBorder(""));
			mPythonCombo.setSelectedIndex(-1);
			mJsonArgumentArea.setText("");
			return;
		} else {
			// Update fields based on the selected effect
			mEffectPanel.setBorder(BorderFactory.createTitledBorder(effect.mId));
			mPythonCombo.setSelectedItem(effect.mScript);
			mJsonArgumentArea.setText(effect.mArgs);
		}
	}
	
	private JPanel getControlPanel() {
		if (mControlPanel == null) {
			mControlPanel = new JPanel();
			mControlPanel.setPreferredSize(new Dimension(150, 35));
			mControlPanel.setLayout(new BoxLayout(mControlPanel, BoxLayout.LINE_AXIS));
			
			mEffectCombo = new JComboBox<>(mEffectModel);
			mEffectCombo.addActionListener(new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					effectSelectionChanged();
				}
			});
			mControlPanel.add(mEffectCombo);
			
			mAddButton = new JButton("Add");
			mAddButton.addActionListener(new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					String newId = JOptionPane.showInputDialog(mAddButton, "Name of the new effect: ", "Effect Name", JOptionPane.QUESTION_MESSAGE);
					// Make that an ID is set
					if (newId == null || newId.isEmpty()) {
						return;
					}
					// Make sure the ID does not yet exist
					for (EffectConfig effect : mEffectEngingeConfig.mEffects) {
						if (effect.mId.equalsIgnoreCase(newId)) {
							JOptionPane.showMessageDialog(mAddButton, "Given name(" + effect.mId + ") allready exists", "Duplicate effect name", JOptionPane.ERROR_MESSAGE);
							return;
						}
					}
					
					EffectConfig newConfig = new EffectConfig();
					newConfig.mId = newId;
					mEffectModel.addElement(newConfig);
					mEffectModel.setSelectedItem(newConfig);
				}
			});
			mControlPanel.add(mAddButton);
			
			mDelButton = new JButton("Del");
			mDelButton.setEnabled(mEffectModel.getSize() > 0);
			mDelButton.addActionListener(new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					if (mEffectModel.getSelectedItem() != null) {
						mEffectModel.removeElement(mEffectModel.getSelectedItem());
					}
					mDelButton.setEnabled(mEffectModel.getSize() > 0);
				}
			});
			mControlPanel.add(mDelButton);
		}
		return mControlPanel;
	}
	
	private JPanel getEffectPanel() {
		if (mEffectPanel == null) {
			mEffectPanel = new JPanel();
			mEffectPanel.setLayout(new BoxLayout(mEffectPanel, BoxLayout.PAGE_AXIS));
			
			JPanel subPanel = new JPanel(new BorderLayout());
			mEffectPanel.add(subPanel);
			
			mPythonLabel = new JLabel("Python: ");
			subPanel.add(mPythonLabel, BorderLayout.WEST);
			
			mPythonCombo = new JComboBox<>(new String[] {"test.py", "rainbow-swirl.py", "rainbow-mood.py"});
//			mPythonCombo.setEditable(true);
			mPythonCombo.setMaximumSize(new Dimension(150, 25));
			subPanel.add(mPythonCombo);
			
			mJsonArgumentLabel = new JLabel("Arguments:");
			mEffectPanel.add(mJsonArgumentLabel);
			
			mJsonArgumentArea = new JTextArea();
			mJsonArgumentArea.setLineWrap(true);
			mJsonArgumentArea.setWrapStyleWord(true);
			mEffectPanel.add(mJsonArgumentArea);
		}
		return mEffectPanel;
	}
}
