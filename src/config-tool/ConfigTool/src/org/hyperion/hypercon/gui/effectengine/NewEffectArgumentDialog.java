package org.hyperion.hypercon.gui.effectengine;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.BorderFactory;
import javax.swing.GroupLayout;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;

import org.hyperion.hypercon.spec.EffectConfig.EffectArg;

/**
 * Dialog for specifying the required parameters (name and type) of an effect argument.
 */
public class NewEffectArgumentDialog extends JDialog {

	/** Return mask for cancellation (cancel-button, closing, etc) */
	public static final int CANCEL_OPTION = 1;
	/** Return mask for accepting (ok-button) */
	public static final int OK_OPTION     = 2;
	
	/** Panel contains the parameters that require configuration by the user */
	private JPanel mArgumentPanel;
	private JLabel mNameLabel;
	private JTextField mNameField;
	private JLabel mTypeLabel;
	private JComboBox<EffectArgumentType> mTypeCombo;
	
	/** Panel contains the controls for the dialog */
	private JPanel mControlPanel;
	private JButton mOkButton;
	private JButton mCancelButton;
	
	/** The current return value for the dialog */
	private int mReturnValue;
	
	/**
	 * Constructs an empty NewEffectArgumentDialog
	 */
	public NewEffectArgumentDialog() {
		super();
		
		initialise();
		
		mNameField.addKeyListener(mKeyListener);
		mTypeCombo.addKeyListener(mKeyListener);
		mOkButton.addKeyListener(mKeyListener);
		mCancelButton.addKeyListener(mKeyListener);
	}
	
	/**
	 * Initialises the dialog, constructing all its sub-panels and components
	 */
	private void initialise() {
		setTitle("New effect argument");
		setResizable(false);
		setSize(320,150);
		setLayout(new BorderLayout());
		setModal(true);
		setDefaultCloseOperation(DISPOSE_ON_CLOSE);
		
		add(getArgumentPanel(), BorderLayout.CENTER);
		add(getControlPanel(),  BorderLayout.SOUTH);
		
		mOkAction.setEnabled(false);
	}
	
	/**
	 * Initialises, if not yet initialised, and returns the argument configuration panel
	 * 
	 * @return The argument configuration panel ({@link #mArgumentPanel})
	 */
	private JPanel getArgumentPanel() {
		if (mArgumentPanel == null) {
			mArgumentPanel = new JPanel();
			mArgumentPanel.setBorder(BorderFactory.createEmptyBorder(5,5,5,5));
			
			mNameLabel = new JLabel("Name: ");
			mArgumentPanel.add(mNameLabel);
			
			mNameField = new JTextField("");
			mNameField.getDocument().addDocumentListener(new DocumentListener() {
				@Override
				public void removeUpdate(DocumentEvent e) {
					documentChanged();
				}
				
				@Override
				public void insertUpdate(DocumentEvent e) {
					documentChanged();
				}
				
				@Override
				public void changedUpdate(DocumentEvent e) {
					documentChanged();
				}
				
				private void documentChanged() {
					String name = mNameField.getText();
					boolean validName = !(name.isEmpty() || name.contains("\""));
					mOkAction.setEnabled(validName);
				}
			});
			mArgumentPanel.add(mNameField);
			
			mTypeLabel = new JLabel("Type: ");
			mArgumentPanel.add(mTypeLabel);
			
			mTypeCombo = new JComboBox<>(EffectArgumentType.values());
			mArgumentPanel.add(mTypeCombo);
			
			GroupLayout layout = new GroupLayout(mArgumentPanel);
			layout.setAutoCreateGaps(true);
			mArgumentPanel.setLayout(layout);
			
			layout.setHorizontalGroup(layout.createSequentialGroup()
					.addGroup(layout.createParallelGroup()
							.addComponent(mNameLabel)
							.addComponent(mTypeLabel))
					.addGroup(layout.createParallelGroup()
							.addComponent(mNameField)
							.addComponent(mTypeCombo)));
			
			layout.setVerticalGroup(layout.createSequentialGroup()
					.addGroup(layout.createParallelGroup()
							.addComponent(mNameLabel)
							.addComponent(mNameField))
					.addGroup(layout.createParallelGroup()
							.addComponent(mTypeLabel)
							.addComponent(mTypeCombo)));
		}
		return mArgumentPanel;
	}
	
	/**
	 * Initialises, if not yet initialised, and returns the dialog control panel
	 * 
	 * @return The dialog control panel ({@link #mControlPanel})
	 */
	private JPanel getControlPanel() {
		if (mControlPanel == null) {
			mControlPanel = new JPanel();
			mControlPanel.setLayout(new FlowLayout(FlowLayout.TRAILING));
			
			mOkButton = new JButton(mOkAction);
			mControlPanel.add(mOkButton);
			
			mCancelButton = new JButton(mCancelAction);
			mControlPanel.add(mCancelButton);
		}
		return mControlPanel;
	}
	
	/**
	 * Shows a new instance of the NewEffectArgumentDialog and constructs anew EffectArg.
	 * 
	 * @return The newly constructed argument (or null if cancelled)
	 */
	public static EffectArg showDialog() {
		NewEffectArgumentDialog dialog = new NewEffectArgumentDialog();
		dialog.mReturnValue = CANCEL_OPTION;
		dialog.setVisible(true);
		if (dialog.mReturnValue == OK_OPTION) {
			String name = dialog.mNameField.getText();
			EffectArgumentType type = (EffectArgumentType) dialog.mTypeCombo.getSelectedItem();

			return new EffectArg(name, type.getDefaultValue());
		}
		
		return null;
	}
	
	private final KeyListener mKeyListener = new KeyListener() {
		@Override
		public void keyTyped(KeyEvent e) {
			if (e.getKeyChar() == KeyEvent.VK_ENTER) {
				if (mOkAction.isEnabled()) {
					mOkAction.actionPerformed(new ActionEvent(e.getSource(), e.getID(), "Ok"));
				}
			} else if (e.getKeyChar() ==  KeyEvent.VK_ESCAPE) {
				mCancelAction.actionPerformed(new ActionEvent(e.getSource(), e.getID(), "Cancel"));
			}
		}
		@Override
		public void keyReleased(KeyEvent e) {
		}
		@Override
		public void keyPressed(KeyEvent e) {
		}
	};
	
	/** Action for handling 'OK' */
	private final Action mOkAction = new AbstractAction("Ok") {
		@Override
		public void actionPerformed(ActionEvent e) {
			mReturnValue = OK_OPTION;
			setVisible(false);
		}
	};

	/** Action for handling 'CANCEL' */
	private final Action mCancelAction = new AbstractAction("Cancel") {
		@Override
		public void actionPerformed(ActionEvent e) {
			mReturnValue = CANCEL_OPTION;
			setVisible(false);
		}
	};
	
	/**
	 * Test entry-point for showing the dialog
	 */
	public static void main(String[] pArgs) {
		NewEffectArgumentDialog.showDialog();
	}
}
