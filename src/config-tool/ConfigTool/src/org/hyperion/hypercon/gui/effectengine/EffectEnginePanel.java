package org.hyperion.hypercon.gui.effectengine;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.beans.Transient;
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
import javax.swing.ListSelectionModel;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.TableColumn;

import org.hyperion.hypercon.spec.EffectConfig;
import org.hyperion.hypercon.spec.EffectConfig.EffectArg;
import org.hyperion.hypercon.spec.EffectEngineConfig;

public class EffectEnginePanel extends JPanel {
	/** The Effect Engine configuration */
	private final EffectEngineConfig mEffectEngingeConfig;
	
	/** The combobox model for selecting an effect configuration */
	private final DefaultComboBoxModel<EffectConfig> mEffectModel;
	
	private final DefaultComboBoxModel<String> mPythonScriptModel;
	
	private JPanel mControlPanel;
	private JComboBox<EffectConfig> mEffectCombo;
	private JButton mCloneButton;
	private JButton mAddButton;
	private JButton mDelButton;
	
	private JPanel mEffectPanel;
	private JLabel mPythonLabel;
	private JTextField mPythonField;
	private JPanel mEffectArgumentPanel;
	private JTable mEffectArgumentTable;
	private JPanel mArgumentControlPanel;
	private JButton mAddArgumentButton;
	private JButton mDelArgumentButton;
	
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
			return effect.mArgs.size();
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
				if (!key.isEmpty()) {
					effect.mArgs.get(rowIndex).key = (String)aValue;
				}
			} else {
				effect.mArgs.get(rowIndex).value = aValue;
			}
		};
	};
	
	public EffectEnginePanel(final EffectEngineConfig pEffectEngineConfig) {
		super();
		
		mEffectEngingeConfig = pEffectEngineConfig;
		mEffectModel = new DefaultComboBoxModel<EffectConfig>(mEffectEngingeConfig.mEffects);
		mPythonScriptModel = new DefaultComboBoxModel<>();
		for (EffectConfig effect : mEffectEngingeConfig.mEffects) {
			if (mPythonScriptModel.getIndexOf(effect.mScript) >= 0) {
				continue;
			}
			mPythonScriptModel.addElement(effect.mScript);
		}

		initialise();
		
		effectSelectionChanged();
	}
	
	private void initialise() {
		setLayout(new BorderLayout());
		
		add(getControlPanel(), BorderLayout.NORTH);
		add(getEffectPanel(), BorderLayout.CENTER);
	}
	
	@Override
	@Transient
	public Dimension getMaximumSize() {
		Dimension maxSize = super.getMaximumSize();
		Dimension prefSize = super.getPreferredSize();
		return new Dimension(maxSize.width, prefSize.height);
	}
	
	private void effectSelectionChanged() {
		EffectConfig effect = (EffectConfig)mEffectModel.getSelectedItem();
		
		// Enable option for the selected effect or disable if none selected
		mEffectPanel.setEnabled(effect != null);
		mPythonLabel.setEnabled(effect != null);
		mPythonField.setEnabled(effect != null);
		mEffectArgumentTable.setEnabled(effect != null);
		
		
		if (effect == null) {
			// Clear all fields
			mPythonField.setText("");
			mEffectArgumentTableModel.fireTableDataChanged();
			return;
		} else {
			// Update fields based on the selected effect
			mPythonField.setText(effect.mScript);
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
					mTextField.selectAll();
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
			
			mPythonField = new JTextField();
			mPythonField.getDocument().addDocumentListener(new DocumentListener() {
				@Override
				public void removeUpdate(DocumentEvent e) {
					update();
				}
				@Override
				public void insertUpdate(DocumentEvent e) {
					update();
				}
				@Override
				public void changedUpdate(DocumentEvent e) {
					update();
				}
				private void update() {
					EffectConfig effect = (EffectConfig) mEffectModel.getSelectedItem();
					if (effect == null) {
						return;
					}
					effect.mScript = mPythonField.getText();
				}
			});
			mPythonField.setMaximumSize(new Dimension(150, 25));
			subPanel.add(mPythonField);

			mEffectArgumentPanel = new JPanel();
			mEffectArgumentPanel.setBorder(BorderFactory.createTitledBorder("Arguments"));
			mEffectArgumentPanel.setLayout(new BorderLayout());
			
			mEffectArgumentTable = new JTable(mEffectArgumentTableModel);
			mEffectArgumentTable.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
			mEffectArgumentPanel.add(new JScrollPane(mEffectArgumentTable), BorderLayout.CENTER);
			
			mEffectArgumentPanel.add(getArgumentControlPanel(), BorderLayout.SOUTH);
			
			mEffectPanel.add(mEffectArgumentPanel);
			
//			mEffectArgumentTable.getColumnModel().getColumn(0).setMaxWidth(100);
			TableColumn col = mEffectArgumentTable.getColumnModel().getColumn(1);
//			col.setMaxWidth(100);
			col.setCellEditor(new EffectArgumentCellEditor());
			col.setCellRenderer(new DefaultTableCellRenderer() {
				@Override
				public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column) {
					if (value instanceof Color) {
						Color color = (Color)value;
						value = String.format("[%d, %d, %d]", color.getRed(), color.getGreen(), color.getBlue());
					}
					return super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);
				}
			});
		}
		return mEffectPanel;
	}
	
	private JPanel getArgumentControlPanel() {
		if (mArgumentControlPanel == null) {
			mArgumentControlPanel = new JPanel();
			
			mAddArgumentButton = new JButton("+");
			mAddArgumentButton.addActionListener(new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					EffectArg newArg = NewEffectArgumentDialog.showDialog();
					if (newArg != null) {
						((EffectConfig)mEffectModel.getSelectedItem()).mArgs.add(newArg);
						int row = ((EffectConfig)mEffectModel.getSelectedItem()).mArgs.size()-1;
						mEffectArgumentTableModel.fireTableRowsInserted(row, row);
					}
				}
			});
			mArgumentControlPanel.add(mAddArgumentButton);
			
			mDelArgumentButton = new JButton("-");
			mDelArgumentButton.addActionListener(new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					int selRow = mEffectArgumentTable.getSelectedRow();
					if (selRow >= 0) {
						((EffectConfig)mEffectModel.getSelectedItem()).mArgs.remove(selRow);
						mEffectArgumentTableModel.fireTableRowsDeleted(selRow, selRow);
					}
				}
			});
			mArgumentControlPanel.add(mDelArgumentButton);
		}
		return mArgumentControlPanel;
	}
}
