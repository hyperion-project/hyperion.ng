package org.hyperion.hypercon.gui;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Vector;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.ComboBoxEditor;
import javax.swing.DefaultComboBoxModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.TableColumn;

import org.hyperion.hypercon.spec.EffectConfig;
import org.hyperion.hypercon.spec.EffectEngineConfig;

public class EffectEnginePanel extends JPanel {

	private final EffectEngineConfig mEffectEngingeConfig;
	
	private final DefaultComboBoxModel<EffectConfig> mEffectModel;
	
	private JPanel mControlPanel;
	private JComboBox<EffectConfig> mEffectCombo;
	private JButton mCloneButton;
	private JButton mAddButton;
	private JButton mDelButton;
	
	private JPanel mEffectPanel;
	private JLabel mPythonLabel;
	private JComboBox<String> mPythonCombo;
	private JPanel mEffectArgumentPanel;
	private JTable mEffectArgumentTable;
	
	private AbstractTableModel mEffectArgumentTableModel = new AbstractTableModel() {
		
		@Override
		public boolean isCellEditable(int rowIndex, int columnIndex) {
			EffectConfig effect = (EffectConfig) mEffectModel.getSelectedItem();
			if (effect == null) {
				return false;
			}
			if (rowIndex == effect.mArgs.size()) {
				return columnIndex == 0;
			}
			return true;
		};
		
		@Override
		public int getColumnCount() {
			return 2;
		}
		
		@Override
		public String getColumnName(int column) {
			switch (column) {
			case 0:
				return "name";
			case 1:
				return "value";
			}
			return "";
		};
		
		@Override
		public int getRowCount() {
			EffectConfig effect = (EffectConfig) mEffectModel.getSelectedItem();
			if (effect == null) {
				return 0;
			}
			return effect.mArgs.size() + 1;
		}
		
		@Override
		public Object getValueAt(int rowIndex, int columnIndex) {
			EffectConfig effect = (EffectConfig) mEffectModel.getSelectedItem();
			if (effect == null) {
				return "";
			}
			if (rowIndex == effect.mArgs.size()) {
				if (columnIndex == 0) {
					return "[key]";
				}
				return "";
			}
			if (columnIndex == 0) {
				return effect.mArgs.get(rowIndex).key;
			} else if (columnIndex == 1){
				return effect.mArgs.get(rowIndex).value;
				
			}
			return "";
		}
		
		@Override
		public void setValueAt(Object aValue, int rowIndex, int columnIndex) {
			EffectConfig effect = (EffectConfig) mEffectModel.getSelectedItem();
			if (effect == null) {
				return;
			}
			
			if (rowIndex == effect.mArgs.size()) {
				String key = aValue.toString().trim();
				if (key.isEmpty() || key.equals("[key]")) {
					return;
				}
				
				effect.mArgs.addElement(new EffectConfig.EffectArg(aValue.toString(), ""));
				return;
			}
			if (columnIndex == 0) {
				String key = aValue.toString().trim();
				if (key.isEmpty()) {
					effect.mArgs.remove(rowIndex);
				} else {
					effect.mArgs.get(rowIndex).key = (String)aValue;
				}
			} else {
				if (aValue instanceof String) {
					// Get the value without any trailing or leading spaces
					String str = ((String)aValue).trim();
					if (str.charAt(0) == '"' && str.charAt(str.length()-1) == '"') {
						// If the string is quoted it is an actual string 
						String actStr = str.substring(1, str.length()-1);
						if (actStr.contains("\"")) {
							// String can not contain quotes
						} else {
							effect.mArgs.get(rowIndex).value = actStr;
						}
					} else {
						// The string is not a string, let's find out what it is
						if (str.equalsIgnoreCase("true") || str.equalsIgnoreCase("false")) {
							// It is a BOOLEAN
							effect.mArgs.get(rowIndex).value = str.equalsIgnoreCase("true");
						} else {
							try {
								int intVal = Integer.parseInt(str);
								// It is an INT
								effect.mArgs.get(rowIndex).value = intVal;
							} catch (Throwable t1) {
								// It was not an integer apparently
								try {
									double doubleVal = Double.parseDouble(str);
									effect.mArgs.get(rowIndex).value = doubleVal;
								} catch (Throwable t2) {
									// It was not an double apparently ....
								}
											
							}
						}
					}
						
				} else {
					effect.mArgs.get(rowIndex).value = aValue;
				}
			}
		};
	};
	
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
		mEffectArgumentTable.setEnabled(effect != null);
		
		
		if (effect == null) {
			// Clear all fields
			mPythonCombo.setSelectedIndex(-1);
			mEffectArgumentTableModel.fireTableDataChanged();
			return;
		} else {
			// Update fields based on the selected effect
			mPythonCombo.setSelectedItem(effect.mScript);
			mEffectArgumentTableModel.fireTableDataChanged();
		}
	}
	
	private JPanel getControlPanel() {
		if (mControlPanel == null) {
			mControlPanel = new JPanel();
			mControlPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 2, 5));
			mControlPanel.setPreferredSize(new Dimension(150, 25));
			mControlPanel.setLayout(new BoxLayout(mControlPanel, BoxLayout.LINE_AXIS));
			
			mEffectCombo = new JComboBox<>(mEffectModel);
			mEffectCombo.setEditable(true);
			mEffectCombo.setEditor(new ComboBoxEditor() {
				private final JTextField mTextField = new JTextField();
				
				private EffectConfig mCurrentEffect = null;
				
				@Override
				public void setItem(Object anObject) {
					if (anObject instanceof EffectConfig) {
						mCurrentEffect = (EffectConfig) anObject;
						mTextField.setText(mCurrentEffect.mId);
					}
				}
				
				@Override
				public void selectAll() {
					if (mCurrentEffect == null) {
						return;
					}
					mTextField.setText(mCurrentEffect.mId);
					mTextField.setSelectionStart(0);
					mTextField.setSelectionEnd(mCurrentEffect.mId.length()-1);
				}
				
				@Override
				public Object getItem() {
					String newId = mTextField.getText().trim();
					if (newId.isEmpty() || newId.contains("\"")) {
						return mCurrentEffect;
					}
					mCurrentEffect.mId = newId;
					return mCurrentEffect;				}
				
				@Override
				public Component getEditorComponent() {
					return mTextField;
				}
				
				private final Vector<ActionListener> mActionListeners = new Vector<>();
				@Override
				public void addActionListener(ActionListener l) {
					mActionListeners.add(l);
				}
				@Override
				public void removeActionListener(ActionListener l) {
					mActionListeners.remove(l);
				}
			});
			mEffectCombo.addActionListener(new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					effectSelectionChanged();
				}
			});
			mControlPanel.add(mEffectCombo);
			
			mCloneButton = new JButton("Clone");
			mCloneButton.addActionListener(new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					EffectConfig effect = (EffectConfig) mEffectModel.getSelectedItem();
					EffectConfig effectClone = effect.clone();
					effectClone.mId += " [clone]";
					mEffectModel.addElement(effectClone);
					mEffectModel.setSelectedItem(effectClone);
				}
			});
			mControlPanel.add(mCloneButton);
			
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
			mEffectPanel.setBorder(BorderFactory.createTitledBorder(""));
			mEffectPanel.setLayout(new BoxLayout(mEffectPanel, BoxLayout.PAGE_AXIS));
			
			JPanel subPanel = new JPanel(new BorderLayout());
			subPanel.setPreferredSize(new Dimension(150, 25));
			subPanel.setMaximumSize(new Dimension(20000, 20));
			mEffectPanel.add(subPanel);
			
			mPythonLabel = new JLabel("Python: ");
			subPanel.add(mPythonLabel, BorderLayout.WEST);
			
			mPythonCombo = new JComboBox<>(new String[] {"test.py", "rainbow-swirl.py", "rainbow-mood.py"});
//			mPythonCombo.setEditable(true);
			mPythonCombo.setMaximumSize(new Dimension(150, 25));
			subPanel.add(mPythonCombo);

			mEffectArgumentPanel = new JPanel();
			mEffectArgumentPanel.setBorder(BorderFactory.createTitledBorder("Arguments"));
			mEffectArgumentPanel.setLayout(new BorderLayout());
			
			mEffectArgumentTable = new JTable(mEffectArgumentTableModel);
			mEffectArgumentPanel.add(new JScrollPane(mEffectArgumentTable));
			
			mEffectPanel.add(mEffectArgumentPanel);
			
			TableColumn col = mEffectArgumentTable.getColumnModel().getColumn(1);
			col.setCellEditor(new EffectArgumentCellEditor());

			mEffectArgumentTable.setCellEditor(new EffectArgumentCellEditor());
		}
		return mEffectPanel;
	}
}
