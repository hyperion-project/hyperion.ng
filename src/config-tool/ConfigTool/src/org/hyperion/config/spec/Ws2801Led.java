package org.hyperion.config.spec;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Graphics2D;

import javax.swing.JComponent;

public class Ws2801Led extends JComponent {

	enum Border {
		top,
		topleft,
		left,
		bottomleft,
		bottom,
		bottomright,
		right,
		topright;
	}
	
	final int size = 12;
	final int halfSize = size/2;
	
	final Border mBorder;
	
	public Ws2801Led(Border pBorder) {
		mBorder = pBorder;
	}

	
	@Override
	protected void paintComponent(Graphics g) {
		Graphics2D g2d = (Graphics2D) g.create();
		
//		g2d.setColor(Color.BLACK);
//		g2d.drawRect(0, 0, getWidth()-1, getHeight()-1);
		
		switch (mBorder) {
		case top:
			g2d.translate(getWidth()/2, getHeight());
			break;
		case topleft:
			g2d.translate(getWidth(), getHeight());
			g2d.rotate(1.75*Math.PI);
			break;
		case left:
			g2d.translate(0, getHeight()/2);
			g2d.rotate(Math.PI*0.5);
			break;
		case bottomleft:
			g2d.translate(getWidth(), 0);
			g2d.rotate(-0.75*Math.PI);
			break;
		case bottom:
			g2d.translate(getWidth()/2, 0);
			g2d.rotate(Math.PI*1.0);
			break;
		case bottomright:
			g2d.rotate(0.75*Math.PI);
			break;
		case right:
			g2d.translate(getWidth(), getHeight()/2);
			g2d.rotate(Math.PI*1.5);
			break;
		case topright:
			g2d.translate(0, getHeight());
			g2d.rotate(0.25*Math.PI);
			break;
		}

		g2d.setColor(new Color(255, 0, 0, 172));
		g2d.fillRoundRect(-3, -12, 6, 6, 2, 2);

		g2d.setColor(new Color(255, 0, 0, 255));
		g2d.drawRoundRect(-3, -12, 6, 6, 2, 2);
		
		g2d.setColor(Color.GRAY);
		g2d.drawRect(-halfSize, -halfSize, size, halfSize);
		g2d.fillRect(-halfSize, -halfSize, size, halfSize);

	}
}
