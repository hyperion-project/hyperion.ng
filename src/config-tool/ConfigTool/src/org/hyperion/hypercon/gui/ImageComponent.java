package org.hyperion.hypercon.gui;

import java.awt.Graphics;
import java.awt.Image;

import javax.swing.JComponent;

public class ImageComponent extends JComponent {

	private Image mImage;
	
	public ImageComponent() {
		super();
	}
	
	public void setImage(Image pImage) {
		mImage = pImage;
	}
	
	@Override
	public void paint(Graphics g) {
		if (mImage == null) {
			return;
		}
		g.drawImage(mImage, 0, 0, getWidth(), getHeight(), null);
	}
}
