package org.hyperion.hypercon.gui;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.HashMap;
import java.util.Map;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.DefaultComboBoxModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JOptionPane;
import javax.swing.JPanel;

import org.hyperion.hypercon.spec.ColorConfig;
import org.hyperion.hypercon.spec.TransformConfig;

public class ColorsPanel extends JPanel {

	private final ColorConfig mColorConfig;
	private final DefaultComboBoxModel<TransformConfig> mTransformsModel;
	
	private JPanel mControlPanel;
	private JComboBox<TransformConfig> mTransformCombo;
	private JButton mAddTransformButton;
	private JButton mDelTransformButton;
	
	private JPanel mTransformPanel;
	
	private final Map<TransformConfig, ColorTransformPanel> mTransformPanels = new HashMap<>();
	
	
	public ColorsPanel(ColorConfig pColorConfig) {
		super();
		
		mColorConfig = pColorConfig;
		mTransformsModel = new DefaultComboBoxModel<TransformConfig>(mColorConfig.mTransforms);
		
		initialise();
	}
	
	private void initialise() {
		setLayout(new BorderLayout(10,10));
		setBorder(BorderFactory.createTitledBorder("Colors"));
		
		add(getControlPanel(), BorderLayout.NORTH);
		
		mTransformPanel = new JPanel();
		mTransformPanel.setLayout(new BorderLayout());
		add(mTransformPanel, BorderLayout.CENTER);
		
		for (TransformConfig config : mColorConfig.mTransforms) {
			mTransformPanels.put(config, new ColorTransformPanel(config));
		}
		ColorTransformPanel currentPanel = mTransformPanels.get(mColorConfig.mTransforms.get(0));
		mTransformPanel.add(currentPanel, BorderLayout.CENTER);
	}
	
	private JPanel getControlPanel() {
		if (mControlPanel == null) {
			mControlPanel = new JPanel();
			mControlPanel.setLayout(new BoxLayout(mControlPanel, BoxLayout.LINE_AXIS));
			
			mTransformCombo = new JComboBox<>(mTransformsModel);
			mTransformCombo.addActionListener(mComboListener);
			mControlPanel.add(mTransformCombo);
			
			mAddTransformButton = new JButton(mAddAction);
			mControlPanel.add(mAddTransformButton);
			
			mDelTransformButton = new JButton(mDelAction);
			mDelTransformButton.setEnabled(mTransformCombo.getItemCount() > 1);
			mControlPanel.add(mDelTransformButton);
		}
		return mControlPanel;
	}
	
	private final Action mAddAction = new AbstractAction("Add") {
		@Override
		public void actionPerformed(ActionEvent e) {
			String newId = JOptionPane.showInputDialog("Give an identifier for the new color-transform:");
			if (newId == null || newId.isEmpty()) {
				// No proper value given
				return;
			}
			
			TransformConfig config = new TransformConfig();
			config.mId = newId;
			
			ColorTransformPanel panel = new ColorTransformPanel(config);
			mTransformPanels.put(config, panel);
			
			mTransformsModel.addElement(config);
			mTransformsModel.setSelectedItem(config);
			
			mDelTransformButton.setEnabled(true);
		}
	};
	private final Action mDelAction = new AbstractAction("Del") {
		@Override
		public void actionPerformed(ActionEvent e) {
			TransformConfig config = (TransformConfig) mTransformCombo.getSelectedItem();
			mTransformPanels.remove(config);
			mTransformsModel.removeElement(config);
			
			mDelTransformButton.setEnabled(mTransformCombo.getItemCount() > 1);
		}
	};
	
	private final ActionListener mComboListener = new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
			TransformConfig selConfig = (TransformConfig) mTransformsModel.getSelectedItem();
			if (selConfig == null) {
				// Something went wrong here, there should always be a selection!
				return;
			}
			
			ColorTransformPanel panel = mTransformPanels.get(selConfig);
			mTransformPanel.removeAll();
			mTransformPanel.add(panel, BorderLayout.CENTER);
			mTransformPanel.revalidate();
			mTransformPanel.repaint();
		}
	};
}
