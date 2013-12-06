package org.hyperion.hypercon.gui;

import java.awt.Color;
import java.awt.Component;

import javax.swing.AbstractCellEditor;
import javax.swing.JCheckBox;
import javax.swing.JColorChooser;
import javax.swing.JSpinner;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.SpinnerNumberModel;
import javax.swing.table.TableCellEditor;

public class EffectArgumentCellEditor extends AbstractCellEditor implements TableCellEditor {

	int selEditor = 0;
	
	private JSpinner mIntegerSpinner = new JSpinner(new SpinnerNumberModel(0, Integer.MIN_VALUE, Integer.MAX_VALUE, 1));
	private JSpinner mDoubleSpinner  = new JSpinner(new SpinnerNumberModel(0.0, -Double.MAX_VALUE, Double.MAX_VALUE, 1.0));
	private JCheckBox mBooleanCheck = new JCheckBox();
	private JColorChooser mColorChooser = new JColorChooser();
	private JTextField mStringEditor = new JTextField();
	
	
	
	@Override
	public Object getCellEditorValue() {
		switch (selEditor) {
		case 0:
			return mIntegerSpinner.getValue();
		case 1:
			return mDoubleSpinner.getValue();
		case 2:
			return mBooleanCheck.isSelected();
		case 3:
			return mStringEditor.getText();
		case 4:
			return mColorChooser.getColor();
		}
		
		return null;
	}

	@Override
	public Component getTableCellEditorComponent(JTable table, Object value, boolean isSelected, int row, int column) {
		if (value instanceof Integer) {
			selEditor = 0;
			mIntegerSpinner.setValue((Integer)value);
			return mIntegerSpinner;
		} else if (value instanceof Double) {
			selEditor = 1;
			mDoubleSpinner.setValue((Double)value);
			return mDoubleSpinner;
		} else if (value instanceof Boolean) {
			selEditor = 2;
			mBooleanCheck.setSelected((Boolean)value);
			return mBooleanCheck;
		} else if (value instanceof Color) {
			selEditor = 4;
//			return mColorCombo;
		}
		
		selEditor = 3;
		mStringEditor.setText('"' + value.toString() + '"');
		return mStringEditor;
	}

}
