import java.io.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.awt.image.*;


public class SimpleImageSteganographer extends Frame {
	private Image carrierImg, payloadImg;
	private BufferedImage carrierImgBuf, payloadImgBuf;

	private Toolkit toolKit;
	private MediaTracker mediaTracker;

	private int carrierImgWidth, carrierImgHeight;
	private int payloadImgWidth, payloadImgHeight;


	public SimpleImageSteganographer(String carrierImgName, String payloadImgName, int numBits) {
		assert((carrierImgName != null): "");

		InitTools();

		if (carrierImgName != null) { carrierImg = toolKit.getImage(carrierImgName); }
		if (payloadImgName != null) { payloadImg = toolKit.getImage(payloadImgName); }

		try {
			if (carrierImg != null) {
				mediaTracker.addImage(carrierImg, 0);
				mediaTracker.waitForID(0);

				carrierImgWidth = carrierImg.getWidth(null);
				carrierImgHeight = carrierImg.getHeight(null);
			}
			if (payloadImg != null) {
				mediaTracker.addImage(payloadImg, 1);
				mediaTracker.waitForID(1);

				payloadImgWidth = payloadImg.getWidth(null);
				payloadImgHeight = payloadImg.getHeight(null);
			}
		} catch (InterruptedException e) {
			Die("error: carrier or payload image loading interrupted");
		}


		if (carrierImgWidth == -1 || payloadImgWidth == -1) {
			Die("error: failed to load carrier or payload image");
		}

		// carrierImg must be at least as large as payloadImg
		if ((carrierImgWidth >= payloadImgWidth) && (carrierImgHeight >= payloadImgHeight)) {
			carrierImgBuf = new BufferedImage(carrierImgWidth, carrierImgHeight, BufferedImage.TYPE_INT_RGB);
			payloadImgBuf = new BufferedImage(payloadImgWidth, payloadImgHeight, BufferedImage.TYPE_INT_RGB);

			(carrierImgBuf.getGraphics()).drawImage(carrierImg, 0, 0, null);
			(payloadImgBuf.getGraphics()).drawImage(payloadImg, 0, 0, null);

			if (payloadImgName == null) {
				ShowPayloadImage(numBits);
			} else {
				HidePayloadImage(numBits);
			}
		} else {
			Die("error: payload image exceeds carrier image dimensions");
		}

		InitGUI();
	}



	public void InitTools() {
		toolKit = Toolkit.getDefaultToolkit();
		mediaTracker = new MediaTracker(this);
	}
	public void InitGUI() {
		addWindowListener(new WindowAdapter() {
      		public void windowClosing(WindowEvent e) {
        		System.exit(0);
      		}
		});

		setSize(carrierImgWidth, carrierImgHeight);
		setTitle("SimpleImageSteganographer");
		show();
	}


	public void HidePayloadImage(int numBits) {
		int xMax = payloadImgWidth;
		int yMax = payloadImgHeight;
		int mask = (1 << numBits) - 1;

		for (int y = 0; y < yMax; y++) {
			for (int x = 0; x < xMax; x++) {
				// getRGB returns 4-byte ARGB integers (eg. 0x00FF00AA)
				// so mask must be shifted to get the MSBs of each byte
				//
				// grab <numBits> left-most bits of each 8-bit payloadImg color byte
				int payImgPxlR = (payloadImgBuf.getRGB(x, y) & (mask << (24 - numBits))) >> (24 - numBits);
				int payImgPxlG = (payloadImgBuf.getRGB(x, y) & (mask << (16 - numBits))) >> (16 - numBits);
				int payImgPxlB = (payloadImgBuf.getRGB(x, y) & (mask << ( 8 - numBits))) >> ( 8 - numBits);

				// overwrite <numBits> right-most bits of each 8-bit carrierImg color byte
				int carImgPxlR = ((((carrierImgBuf.getRGB(x, y) & 0x00FF0000) >> 16) >> numBits) << numBits) | payImgPxlR;
				int carImgPxlG = ((((carrierImgBuf.getRGB(x, y) & 0x0000FF00) >>  8) >> numBits) << numBits) | payImgPxlG;
				int carImgPxlB = ((((carrierImgBuf.getRGB(x, y) & 0x000000FF) >>  0) >> numBits) << numBits) | payImgPxlB;

				// uncomment this to show carrierImg with payloadImg hidden in it
				carrierImgBuf.setRGB(x, y, ((carImgPxlR << 16) | (carImgPxlG << 8) | (carImgPxlB << 0)));
				// uncomment this to show color-reduced hidden image
				// carrierImgBuf.setRGB(x, y, (((payImgPxlR << (8 - numBits)) << 16) | ((payImgPxlG << (8 - numBits)) << 8) | ((payImgPxlB << (8 - numBits)))));
			}
		}
	}

	public void ShowPayloadImage(int numBits) {
		int xMax = carrierImgWidth;
		int yMax = carrierImgHeight;
		int mask = (1 << numBits) - 1;

		for (int y = 0; y < yMax; y++) {
			for (int x = 0; x < xMax; x++) {
				// filter <numBits> right-most bits of each 8-bit carrierImg color byte
				int carImgPxlR = (((carrierImgBuf.getRGB(x, y) & 0x00FF0000) >> 16) & mask) << (8 - numBits);
				int carImgPxlG = (((carrierImgBuf.getRGB(x, y) & 0x0000FF00) >>  8) & mask) << (8 - numBits);
				int carImgPxlB = (((carrierImgBuf.getRGB(x, y) & 0x000000FF) >>  0) & mask) << (8 - numBits);

				// uncomment this to show payloadImg hidden in carrierImg
				carrierImgBuf.setRGB(x, y, ((carImgPxlR << 16) | (carImgPxlG << 8) | (carImgPxlB << 0)));
			}
		}
	}

	public void paint(Graphics graphics) {
		if (carrierImgBuf != null) {
			graphics.drawImage(carrierImgBuf, 0, 0, null);
		}
	}



	public static void Die(String s) {
		System.out.println(s);
		System.exit(1);
	}

	public static void main(String[] args) {
		switch (args.length) {
			case 3: {
				if (args[0].equals("-show")) {
					// show payload image in (suspected) carrier image
					try {
						int numBits = Integer.parseInt(args[2]);

						if (numBits >= 0 && numBits <= 8) {
							new SimpleImageSteganographer(args[1], null, numBits);
						} else {
							Die("[main] " + args[0] + ": illegal number of bits");
						}
					} catch(NumberFormatException e) {
						Die("[main] " + args[0] + ": argument not a number");
					}
				}
			} break;

			case 4: {
				if (args[0].equals("-hide")) {
					// hide payload image in carrier image
					try {
						int numBits = Integer.parseInt(args[3]);

						if (numBits >= 0 && numBits <= 8) {
							new SimpleImageSteganographer(args[1], args[2], numBits);
						} else {
							Die("[main] " + args[0] + ": illegal number of bits");
						}
					} catch (NumberFormatException e) {
						Die("[main] " + args[0] + ": argument not a number");
					}
				}
			} break;

			default: {
				Die("usage: java SimpleImageSteganographer <-hide | -show> <carrierImage <payloadImage>> <numBitsPerSubPixel>");
			} break;
		}
	}
}

