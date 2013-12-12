package org.hyperion.hypercon.gui.effectengine;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JColorChooser;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPopupMenu;

public class ColorPicker extends JComponent {

	private JLabel mLabel;
	private JButton mButton;
	
	JPopupMenu mPopup = new JPopupMenu();
	JColorChooser mColorChooser = new JColorChooser();
	
	private Color mSelectedColor = Color.RED;
	
	public ColorPicker() {
		super();
		
		initialise();
	}
	
	private void initialise() {
		setLayout(new BorderLayout());
		
		mLabel = new JLabel("[23, 123, 1]");
		add(mLabel, BorderLayout.CENTER);
		
		mButton = new JButton();
		mButton.setPreferredSize(new Dimension(25, 25));
		add(mButton, BorderLayout.EAST);
		
		mPopup.add(mColorChooser);
		mButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				Color newColor = JColorChooser.showDialog(ColorPicker.this, "Select a color", mSelectedColor);
				if (newColor == null) {
					return;
				}
				setColor(newColor);
			}
		});
	}
	
	public void setColor(Color pColor) {
		mSelectedColor = pColor;
		mLabel.setText(String.format("[%d, %d, %d]", mSelectedColor.getRed(), mSelectedColor.getGreen(), mSelectedColor.getBlue()));
	}
	
	public Color getColor() {
		return mSelectedColor;
	}
	
	public static void main(String[] pArgs) {
		JFrame frame = new JFrame();
		frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		frame.setSize(240, 100);

		frame.getContentPane().setLayout(new BorderLayout());
		frame.getContentPane().add(new ColorPicker());
		
		frame.setVisible(true);
	}
}
