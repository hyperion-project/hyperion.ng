#ifndef LEDEVICELPD8806_H
#define LEDEVICELPD8806_H

// Local hyperion includes
#include "ProviderSpi.h"

///
/// Implementation of the LedDevice interface for writing to LPD8806 led device.
///
/// The following description is copied from 'adafruit' (github.com/adafruit/LPD8806)
///
/// Clearing up some misconceptions about how the LPD8806 drivers work:
///
/// The LPD8806 is not a FIFO shift register.  The first data out controls the
/// LED *closest* to the processor (unlike a typical shift register, where the
/// first data out winds up at the *furthest* LED).  Each LED driver 'fills up'
/// with data and then passes through all subsequent bytes until a latch
/// condition takes place.  This is actually pretty common among LED drivers.
///
/// All color data bytes have the high bit (128) set, with the remaining
/// seven bits containing a brightness value (0-127).  A byte with the high
/// bit clear has special meaning (explained later).
///
/// The rest gets bizarre...
///
/// The LPD8806 does not perform an in-unison latch (which would display the
/// newly-transmitted data all at once).  Rather, each individual byte (even
/// the separate G, R, B components of each LED) is latched AS IT ARRIVES...
/// or more accurately, as the first bit of the subsequent byte arrives and
/// is passed through.  So the strip actually refreshes at the speed the data
/// is issued, not instantaneously (this can be observed by greatly reducing
/// the data rate).  This has implications for POV displays and light painting
/// applications.  The 'subsequent' rule also means that at least one extra
/// byte must follow the last pixel, in order for the final blue LED to latch.
///
/// To reset the pass-through behaviour and begin sending new data to the start
/// of the strip, a number of zero bytes must be issued (remember, all color
/// data bytes have the high bit set, thus are in the range 128 to 255, so the
/// zero is 'special').  This should be done before each full payload of color
/// values to the strip.  Curiously, zero bytes can only travel one meter (32
/// LEDs) down the line before needing backup; the next meter requires an
/// extra zero byte, and so forth.  Longer strips will require progressively
/// more zeros.  *(see note below)
///
/// In the interest of efficiency, it's possible to combine the former EOD
/// extra latch byte and the latter zero reset...the same data can do double
/// duty, latching the last blue LED while also resetting the strip for the
/// next payload.
///
/// So: reset byte(s) of suitable length are issued once at startup to 'prime'
/// the strip to a known ready state.  After each subsequent LED color payload,
/// these reset byte(s) are then issued at the END of each payload, both to
/// latch the last LED and to prep the strip for the start of the next payload
/// (even if that data does not arrive immediately).  This avoids a tiny bit
/// of latency as the new color payload can begin issuing immediately on some
/// signal, such as a timer or GPIO trigger.
///
/// Technically these zero byte(s) are not a latch, as the color data (save
/// for the last byte) is already latched.  It's a start-of-data marker, or
/// an indicator to clear the thing-that's-not-a-shift-register.  But for
/// conversational consistency with other LED drivers, we'll refer to it as
/// a 'latch' anyway.
///
/// This has been validated independently with multiple customers'
/// hardware.  Please do not report as a bug or issue pull requests for
/// this.  Fewer zeros sometimes gives the *illusion* of working, the first
/// payload will correctly load and latch, but subsequent frames will drop
/// data at the end.  The data shortfall won't always be visually apparent
/// depending on the color data loaded on the prior and subsequent frames.
/// Tested.  Confirmed.  Fact.
///
///
/// The summary of the story is that the following needs to be written on the spi-device:
/// 1RRRRRRR 1GGGGGGG 1BBBBBBB 1RRRRRRR 1GGGGGGG ... ... 1GGGGGGG 1BBBBBBB 00000000 00000000 ...
/// |---------led_1----------| |---------led_2--         -led_n----------| |----clear data--
///
/// The number of zeroes in the 'clear data' is (#led/32 + 1)bytes (or *8 for bits)
///
class LedDeviceLpd8806 : public ProviderSpi
{
public:

	///
	/// @brief Constructs a LDP8806 LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceLpd8806(const QJsonObject &deviceConfig);

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
	static LedDevice* construct(const QJsonObject &deviceConfig);

private:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;
};

#endif // LEDEVICELPD8806_H
