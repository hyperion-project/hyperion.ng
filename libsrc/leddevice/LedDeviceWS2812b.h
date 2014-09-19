#ifndef LEDDEVICEWS2812B_H_
#define LEDDEVICEWS2812B_H_

#pragma once


// Set tabs to 4 spaces.

// =================================================================================================
//
//		 __      __  _________________   ______  ____________   ____________________.__
//		/  \    /  \/   _____/\_____  \ /  __  \/_   \_____  \  \______   \______   \__|
//		\   \/\/   /\_____  \  /  ____/ >      < |   |/  ____/   |       _/|     ___/  |
//		 \        / /        \/       \/   --   \|   /       \   |    |   \|    |   |  |
//		  \__/\  / /_______  /\_______ \______  /|___\_______ \  |____|_  /|____|   |__|
//		       \/          \/         \/      \/             \/         \/
//
// WS2812 NeoPixel driver
// Based on code by Richard G. Hirst and others
// Adapted for the WS2812 by 626Pilot, April/May 2014
// See: https://github.com/626Pilot/RaspberryPi-NeoPixel-WS2812
// Version: https://github.com/626Pilot/RaspberryPi-NeoPixel-WS2812/blob/1d43407d9e6eba19bff24330bc09a27963b55751/ws2812-RPi.c
// Huge ASCII art section labels are from http://patorjk.com/software/taag/
//
// LED driver adaptation by Kammerjaeger ()
// mostly code removed that was not needed
//
// License: GPL
//
// You are using this at your OWN RISK. I believe this software is reasonably safe to use (aside
// from the intrinsic risk to those who are photosensitive - see below), although I can't be certain
// that it won't trash your hardware or cause property damage.
//
// Speaking of risk, WS2812 pixels are bright enough to cause eye pain and (for all I know) possibly
// retina damage when run at full strength. It's a good idea to set the brightness at 0.2 or so for
// direct viewing (whether you're looking directly at the pixels or not), or to put some diffuse
// material between you and the LEDs.
//
// PHOTOSENSITIVITY WARNING:
// Patterns of light and darkness (stationary or moving), flashing lights, patterns and backgrounds
// on screens, and the like, may cause epilleptic seizures in some people. This is a danger EVEN IF
// THE PERSON (WHICH MAY BE *YOU*) HAS NEVER KNOWINGLY HAD A PHOTOSENSITIVE EPISODE BEFORE. It's up
// to you to learn the warning signs, but symptoms may include dizziness, nausea, vision changes,
// convlusions, disorientation, involuntary movements, and eye twitching. (This list is not
// necessarily exhaustive.)
//
// NEOPIXEL BEST PRACTICES: https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices
//
// Connections:
//		Positive to Raspberry Pi's 3.3v, for better separation connect only ground and data directly
//					(5v can be used then without a problem, at least it worked for me, Kammerjaeger)
//		Negative to Raspberry Pi's ground
// 		Data to GPIO18 (Pin 12) (through a resistor, which you should know from the Best
// 		Practices guide!)
//
//    Buy WS2812-based stuff from: http://adafruit.com
//
//  To activate: use led device "ws2812s" in the hyperion configuration
//                                 (it needs to be root so it can map the peripherals' registers)
//
// =================================================================================================

// This is for the WS2812 LEDs. It won't work with the older WS2811s, although it could be modified
// for that without too much trouble. Preliminary driver used Frank Buss' servo driver, but I moved
// to Richard Hirst's memory mapping/access model because his code already works with DMA, and has
// what I think is a slightly cleaner way of accessing the registers: register[name] rather than
// *(register + name).

// At the time of writing, there's a lot of confusing "PWM DMA" code revolving around simulating
// an FM signal. Usually this is done without properly initializing certain registers, which is
// OK for their purpose, but I needed to be able to transfer actual coherent data and have it wind
// up in a proper state once it was transferred. This has proven to be a somewhat painful task.
// The PWM controller likes to ignore the RPTL1 bit when the data is in a regular, repeating
// pattern. I'M NOT MAKING IT UP! It really does that. It's bizarre. There are lots of other
// strange irregularities as well, which had to be figured out through trial and error. It doesn't
// help that the BCM2835 ARM Peripherals manual contains outright errors and omissions!

// Many examples of this kind of code have magic numbers in them. If you don't know, a magic number
// is one that either lacks an obvious structure (e.g. 0x2020C000) or purpose. Please don't use
// that stuff in any code you release! All magic numbers found in reference code have been changed
// to DEFINEs. That way, instead of seeing some inscrutable number, you see (e.g.) PWM_CTL.

// References - BCM2835 ARM Peripherals:
//              http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
//
//              Raspberry Pi low-level peripherals:
//              http://elinux.org/RPi_Low-level_peripherals
//
//				Richard Hirst's nice, clean code:
//				https://github.com/richardghirst/PiBits/blob/master/PiFmDma/PiFmDma.c
//
//              PWM clock register:
//              http://www.raspberrypi.org/forums/viewtopic.php?t=8467&p=124620
//
//				Simple (because it's in assembly) PWM+DMA setup:
//				https://github.com/mikedurso/rpi-projects/blob/master/asm-nyancat/rpi-nyancat.s
//
//				Adafruit's NeoPixel driver:
//				https://github.com/adafruit/Adafruit_NeoPixel/blob/master/Adafruit_NeoPixel.cpp

// Hyperion includes
#include <leddevice/LedDevice.h>

//#define BENCHMARK
#define WS2812_ASM_OPTI

// The page map contains pointers to memory that we will allocate below. It uses two pointers
// per address. This is because the software (this program) deals only in virtual addresses,
// whereas the DMA controller can only access RAM via physical address. (If that's not confusing
// enough, it writes to peripherals by their bus addresses.)
struct page_map_t
{
	uint8_t *virtaddr;
	uint32_t physaddr;
};

// Control Block (CB) - this tells the DMA controller what to do.
struct dma_cb_t
{
	unsigned	info;		// Transfer Information (TI)
	unsigned	src;		// Source address (physical)
	unsigned	dst;		// Destination address (bus)
	unsigned	length;		// Length in bytes (not words!)
	unsigned	stride;		// We don't care about this
	unsigned	next;		// Pointer to next control block
	unsigned	pad[2];		// These are "reserved" (unused)
};

///
/// Implementation of the LedDevice interface for writing to Ws2801 led device.
///
class LedDeviceWS2812b : public LedDevice
{
public:
	///
	/// Constructs the LedDevice for a string containing leds of the type WS2812
	LedDeviceWS2812b();

	~LedDeviceWS2812b();
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

	/// Switch the leds off
	virtual int switchOff();

private:

	/// the number of leds (needed when switching off)
	size_t mLedCount;

	page_map_t *page_map;						// This will hold the page map, which we'll allocate
	uint8_t *virtbase;					// Pointer to some virtual memory that will be allocated

	volatile unsigned int *pwm_reg;		// PWM controller register set
	volatile unsigned int *clk_reg;		// PWM clock manager register set
	volatile unsigned int *dma_reg;		// DMA controller register set
	volatile unsigned int *gpio_reg;		// GPIO pin controller register set

	// Contains arrays of control blocks and their related samples.
	// One pixel needs 72 bits (24 bits for the color * 3 to represent them on the wire).
	// 		 768 words = 341.3 pixels
	// 		1024 words = 455.1 pixels
	// The highest I can make this number is 1016. Any higher, and it will start copying garbage to the
	// PWM controller. I think it might be because of the virtual->physical memory mapping not being
	// contiguous, so *pointer+1016 isn't "next door" to *pointer+1017 for some weird reason.
	// However, that's still enough for 451.5 color instructions! If someone has more pixels than that
	// to control, they can figure it out. I tried Hirst's message of having one CB per word, which
	// seems like it might fix that, but I couldn't figure it out.
	#define NUM_DATA_WORDS 1016
	struct control_data_s {
		dma_cb_t cb[1];
		uint32_t sample[NUM_DATA_WORDS];
	};

	//struct control_data_s *ctl;

	// PWM waveform buffer (in words), 16 32-bit words are enough to hold 170 wire bits.
	// That's OK if we only transmit from the FIFO, but for DMA, we will use a much larger size.
	// 1024 (4096 bytes) should be enough for over 400 elements. It can be bumped up if you need more!
	unsigned int PWMWaveform[NUM_DATA_WORDS];

	void initHardware();
	void startTransfer();

	void clearPWMBuffer();
	void setPWMBit(unsigned int bitPos, unsigned char bit);

	unsigned int mem_phys_to_virt(uint32_t phys);
	unsigned int mem_virt_to_phys(void *virt);
	void terminate(int dummy);
	void fatal(const char *fmt, ...);
	void * map_peripheral(uint32_t base, uint32_t len);
	void printBinary(unsigned int i, unsigned int bits);

#ifdef BENCHMARK
	unsigned int runCount;
	long combinedNseconds;
	long shortestNseconds;
#endif
};












#endif /* LEDDEVICEWS2812B_H_ */
