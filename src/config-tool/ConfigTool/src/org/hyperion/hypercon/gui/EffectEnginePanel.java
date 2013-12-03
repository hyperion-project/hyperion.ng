package org.hyperion.hypercon.gui;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextArea;

import org.hyperion.hypercon.spec.EffectConfig;
import org.hyperion.hypercon.spec.EffectEngineConfig;

public class EffectEnginePanel extends JPanel {

	private final EffectEngineConfig mEffectEngingeConfig;
	
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
		
		initialise();
	}
	
	private void initialise() {
		setLayout(new BorderLayout());
		
		add(getControlPanel(), BorderLayout.NORTH);
		add(getEffectPanel(), BorderLayout.CENTER);
	}
	
	private JPanel getControlPanel() {
		if (mControlPanel == null) {
			mControlPanel = new JPanel();
			mControlPanel.setPreferredSize(new Dimension(150, 35));
			mControlPanel.setLayout(new BoxLayout(mControlPanel, BoxLayout.LINE_AXIS));
			
			mEffectCombo = new JComboBox<>(mEffectEngingeConfig.mEffects);
			mEffectCombo.addActionListener(new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					
				}
			});
			mControlPanel.add(mEffectCombo);
			
			mAddButton = new JButton("Add");
			mControlPanel.add(mAddButton);
			
			mDelButton = new JButton("Del");
			mControlPanel.add(mDelButton);
		}
		return mControlPanel;
	}
	
	private JPanel getEffectPanel() {
		if (mEffectPanel == null) {
			mEffectPanel = new JPanel();
			mEffectPanel.setBorder(BorderFactory.createTitledBorder("test-slow"));
			mEffectPanel.setLayout(new BoxLayout(mEffectPanel, BoxLayout.PAGE_AXIS));
			
			JPanel subPanel = new JPanel(new BorderLayout());
			mEffectPanel.add(subPanel);
			
			mPythonLabel = new JLabel("Python: ");
			subPanel.add(mPythonLabel, BorderLayout.WEST);
			
			mPythonCombo = new JComboBox<>(new String[] {"test.py", "rainbow-swirl.py", "rainbow-mood.py"});
			mPythonCombo.setMaximumSize(new Dimension(150, 25));
			subPanel.add(mPythonCombo);
			
			mJsonArgumentLabel = new JLabel("Arguments:");
			mEffectPanel.add(mJsonArgumentLabel);
			
			mJsonArgumentArea = new JTextArea();
			mEffectPanel.add(mJsonArgumentArea);
		}
		return mEffectPanel;
	}
}
