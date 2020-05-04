---
sidebarDepth: 2
---

# LED Hardware
Hyperion supports a lot of different controllers and led chips. Also network communication is possible, therefore we also support Philips Hue, AtmoOrb and more.

## General Settings
Applicable for all led hardware implementations \
  * RGB byte order: If you want to check this value, use the wizard.
  * Refresh time: If no source is active and the led hardware component is enabled, this will update by the given interval time the led hardware with black color.

## Specific Settings
Each LED hardware has specific settings which are explained here

### SPI
Are 4 wire leds which can be powered via SPI of a Raspberry Pi or an Ardunio (which is usb connected to your computer/HTPC/Pi)

#### apa102
APA 102. These LEDs are known for a good color spectrum (converting a data signal to the wanted color).

#### ws2801
The color spectrum of these leds is bad.

#### lpd6803
#### lpd8806
#### p9813
#### sk6812spi
The SK6812 are **3** wire leds, you could also drive them via spi.

#### sk6822spi
The SK6822 are **3** wire leds, you could also drive them via spi.

#### ws2812spi
The WS2812 are **3** wire leds, you could also drive them via spi.

### USB
Plug and play. The following controllers are supported.

#### adalight
Most used because it's cheap and easy! An Arduino powered by an adalight sketch. We provide a modified version of it. Checkout TUTORIAL

#### atmo

#### dmx
#### file
#### hyperionusbasp
#### lightpack
#### multilightpack
#### paintpack
#### rawhid
#### sedu
#### tpm2

### Network
Hue bridges, nodeMCU, AtmoOrbs everything that is reachable over network.

#### philipshue
The well known [Philips Hue Bridge + Bulbs](https://www.amazon.com/s/ref=nb_sb_noss?url=search-alias%3Daps&field-keywords=philips+hue+starter+set&rh=i%3Aaps%2Ck%3Aphilips+hue+starter+set&tag=hyperionpro05-20) is supported. How to configure them with Hyperion? Checkout: Web configuration

##### Configuration Tips & Tricks
  * Use the Philips Hue wizard at the web configuration for configuration (available at led hardware section)!
  * Color calibration is not required, you can keep the default values.
  * If the bightness is to low for you and Hyperion is already at 100% you can higher the brightness factor at the web configuration -> LED hardware
  * Brightness componsation influences the brightness across different color (Adjust at the color section)
  * To enable/disable the bridge control from Hyperion, disable Hyperion or just the led hardware component. The previous lamp state will be recovered
#### atmoorb
#### tpm2net
#### udpe131
#### udph801
#### udpraw
#### tinkerforge
#### fadecandy

### Raspberry Pi
Just for the Raspberry Pi!
#### ws281x
Driving all kinds of WS281X stripes. Due to mixed feedback we recommend adalight or Raspberry Pi SPI.
#### piblaster
[PiBlaster on Github](https://github.com/sarfata/pi-blaster)

