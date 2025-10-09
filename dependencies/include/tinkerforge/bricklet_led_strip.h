/* ***********************************************************
 * This file was automatically generated on 2023-11-30.      *
 *                                                           *
 * C/C++ Bindings Version 2.1.33                             *
 *                                                           *
 * If you have a bugfix for this file and want to commit it, *
 * please fix the bug in the generator. You can find a link  *
 * to the generators git repository on tinkerforge.com       *
 *************************************************************/

#ifndef BRICKLET_LED_STRIP_H
#define BRICKLET_LED_STRIP_H

#include "ip_connection.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup BrickletLEDStrip LED Strip Bricklet
 */

/**
 * \ingroup BrickletLEDStrip
 *
 * Controls up to 320 RGB LEDs
 */
typedef Device LEDStrip;

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_SET_RGB_VALUES 1

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_GET_RGB_VALUES 2

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_SET_FRAME_DURATION 3

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_GET_FRAME_DURATION 4

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_GET_SUPPLY_VOLTAGE 5

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_SET_CLOCK_FREQUENCY 7

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_GET_CLOCK_FREQUENCY 8

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_SET_CHIP_TYPE 9

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_GET_CHIP_TYPE 10

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_SET_RGBW_VALUES 11

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_GET_RGBW_VALUES 12

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_SET_CHANNEL_MAPPING 13

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_GET_CHANNEL_MAPPING 14

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_ENABLE_FRAME_RENDERED_CALLBACK 15

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_DISABLE_FRAME_RENDERED_CALLBACK 16

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_IS_FRAME_RENDERED_CALLBACK_ENABLED 17

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_FUNCTION_GET_IDENTITY 255

/**
 * \ingroup BrickletLEDStrip
 *
 * Signature: \code void callback(uint16_t length, void *user_data) \endcode
 *
 * This callback is triggered directly after a new frame is rendered. The
 * parameter is the number of RGB or RGBW LEDs in that frame.
 *
 * You should send the data for the next frame directly after this callback
 * was triggered.
 *
 * For an explanation of the general approach see {@link led_strip_set_rgb_values}.
 */
#define LED_STRIP_CALLBACK_FRAME_RENDERED 6


/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHIP_TYPE_WS2801 2801

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHIP_TYPE_WS2811 2811

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHIP_TYPE_WS2812 2812

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHIP_TYPE_LPD8806 8806

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHIP_TYPE_APA102 102

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_RGB 6

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_RBG 9

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_BRG 33

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_BGR 36

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_GRB 18

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_GBR 24

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_RGBW 27

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_RGWB 30

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_RBGW 39

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_RBWG 45

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_RWGB 54

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_RWBG 57

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_GRWB 78

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_GRBW 75

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_GBWR 108

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_GBRW 99

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_GWBR 120

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_GWRB 114

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_BRGW 135

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_BRWG 141

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_BGRW 147

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_BGWR 156

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_BWRG 177

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_BWGR 180

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_WRBG 201

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_WRGB 198

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_WGBR 216

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_WGRB 210

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_WBGR 228

/**
 * \ingroup BrickletLEDStrip
 */
#define LED_STRIP_CHANNEL_MAPPING_WBRG 225

/**
 * \ingroup BrickletLEDStrip
 *
 * This constant is used to identify a LED Strip Bricklet.
 *
 * The {@link led_strip_get_identity} function and the
 * {@link IPCON_CALLBACK_ENUMERATE} callback of the IP Connection have a
 * \c device_identifier parameter to specify the Brick's or Bricklet's type.
 */
#define LED_STRIP_DEVICE_IDENTIFIER 231

/**
 * \ingroup BrickletLEDStrip
 *
 * This constant represents the display name of a LED Strip Bricklet.
 */
#define LED_STRIP_DEVICE_DISPLAY_NAME "LED Strip Bricklet"

/**
 * \ingroup BrickletLEDStrip
 *
 * Creates the device object \c led_strip with the unique device ID \c uid and adds
 * it to the IPConnection \c ipcon.
 */
void led_strip_create(LEDStrip *led_strip, const char *uid, IPConnection *ipcon);

/**
 * \ingroup BrickletLEDStrip
 *
 * Removes the device object \c led_strip from its IPConnection and destroys it.
 * The device object cannot be used anymore afterwards.
 */
void led_strip_destroy(LEDStrip *led_strip);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns the response expected flag for the function specified by the
 * \c function_id parameter. It is *true* if the function is expected to
 * send a response, *false* otherwise.
 *
 * For getter functions this is enabled by default and cannot be disabled,
 * because those functions will always send a response. For callback
 * configuration functions it is enabled by default too, but can be disabled
 * via the led_strip_set_response_expected function. For setter functions it is
 * disabled by default and can be enabled.
 *
 * Enabling the response expected flag for a setter function allows to
 * detect timeouts and other error conditions calls of this setter as well.
 * The device will then send a response for this purpose. If this flag is
 * disabled for a setter function then no response is sent and errors are
 * silently ignored, because they cannot be detected.
 */
int led_strip_get_response_expected(LEDStrip *led_strip, uint8_t function_id, bool *ret_response_expected);

/**
 * \ingroup BrickletLEDStrip
 *
 * Changes the response expected flag of the function specified by the
 * \c function_id parameter. This flag can only be changed for setter
 * (default value: *false*) and callback configuration functions
 * (default value: *true*). For getter functions it is always enabled.
 *
 * Enabling the response expected flag for a setter function allows to detect
 * timeouts and other error conditions calls of this setter as well. The device
 * will then send a response for this purpose. If this flag is disabled for a
 * setter function then no response is sent and errors are silently ignored,
 * because they cannot be detected.
 */
int led_strip_set_response_expected(LEDStrip *led_strip, uint8_t function_id, bool response_expected);

/**
 * \ingroup BrickletLEDStrip
 *
 * Changes the response expected flag for all setter and callback configuration
 * functions of this device at once.
 */
int led_strip_set_response_expected_all(LEDStrip *led_strip, bool response_expected);

/**
 * \ingroup BrickletLEDStrip
 *
 * Registers the given \c function with the given \c callback_id. The
 * \c user_data will be passed as the last parameter to the \c function.
 */
void led_strip_register_callback(LEDStrip *led_strip, int16_t callback_id, void (*function)(void), void *user_data);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns the API version (major, minor, release) of the bindings for this
 * device.
 */
int led_strip_get_api_version(LEDStrip *led_strip, uint8_t ret_api_version[3]);

/**
 * \ingroup BrickletLEDStrip
 *
 * Sets *length* RGB values for the LEDs starting from *index*.
 *
 * To make the colors show correctly you need to configure the chip type
 * ({@link led_strip_set_chip_type}) and a 3-channel channel mapping ({@link led_strip_set_channel_mapping})
 * according to the connected LEDs.
 *
 * Example: If you set
 *
 * * index to 5,
 * * length to 3,
 * * r to [255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
 * * g to [0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] and
 * * b to [0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
 *
 * the LED with index 5 will be red, 6 will be green and 7 will be blue.
 *
 * \note Depending on the LED circuitry colors can be permuted.
 *
 * The colors will be transfered to actual LEDs when the next
 * frame duration ends, see {@link led_strip_set_frame_duration}.
 *
 * Generic approach:
 *
 * * Set the frame duration to a value that represents
 *   the number of frames per second you want to achieve.
 * * Set all of the LED colors for one frame.
 * * Wait for the {@link LED_STRIP_CALLBACK_FRAME_RENDERED} callback.
 * * Set all of the LED colors for next frame.
 * * Wait for the {@link LED_STRIP_CALLBACK_FRAME_RENDERED} callback.
 * * and so on.
 *
 * This approach ensures that you can change the LED colors with
 * a fixed frame rate.
 *
 * The actual number of controllable LEDs depends on the number of free
 * Bricklet ports. See :ref:`here <led_strip_bricklet_ram_constraints>` for more
 * information. A call of {@link led_strip_set_rgb_values} with index + length above the
 * bounds is ignored completely.
 */
int led_strip_set_rgb_values(LEDStrip *led_strip, uint16_t index, uint8_t length, uint8_t r[16], uint8_t g[16], uint8_t b[16]);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns *length* R, G and B values starting from the
 * given LED *index*.
 *
 * The values are the last values that were set by {@link led_strip_set_rgb_values}.
 */
int led_strip_get_rgb_values(LEDStrip *led_strip, uint16_t index, uint8_t length, uint8_t ret_r[16], uint8_t ret_g[16], uint8_t ret_b[16]);

/**
 * \ingroup BrickletLEDStrip
 *
 * Sets the frame duration.
 *
 * Example: If you want to achieve 20 frames per second, you should
 * set the frame duration to 50ms (50ms * 20 = 1 second).
 *
 * For an explanation of the general approach see {@link led_strip_set_rgb_values}.
 */
int led_strip_set_frame_duration(LEDStrip *led_strip, uint16_t duration);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns the frame duration as set by {@link led_strip_set_frame_duration}.
 */
int led_strip_get_frame_duration(LEDStrip *led_strip, uint16_t *ret_duration);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns the current supply voltage of the LEDs.
 */
int led_strip_get_supply_voltage(LEDStrip *led_strip, uint16_t *ret_voltage);

/**
 * \ingroup BrickletLEDStrip
 *
 * Sets the frequency of the clock.
 *
 * The Bricklet will choose the nearest achievable frequency, which may
 * be off by a few Hz. You can get the exact frequency that is used by
 * calling {@link led_strip_get_clock_frequency}.
 *
 * If you have problems with flickering LEDs, they may be bits flipping. You
 * can fix this by either making the connection between the LEDs and the
 * Bricklet shorter or by reducing the frequency.
 *
 * With a decreasing frequency your maximum frames per second will decrease
 * too.
 *
 * \note
 *  The frequency in firmware version 2.0.0 is fixed at 2MHz.
 *
 * .. versionadded:: 2.0.1$nbsp;(Plugin)
 */
int led_strip_set_clock_frequency(LEDStrip *led_strip, uint32_t frequency);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns the currently used clock frequency as set by {@link led_strip_set_clock_frequency}.
 *
 * .. versionadded:: 2.0.1$nbsp;(Plugin)
 */
int led_strip_get_clock_frequency(LEDStrip *led_strip, uint32_t *ret_frequency);

/**
 * \ingroup BrickletLEDStrip
 *
 * Sets the type of the LED driver chip. We currently support the chips
 *
 * * WS2801,
 * * WS2811,
 * * WS2812 / SK6812 / NeoPixel RGB,
 * * SK6812RGBW / NeoPixel RGBW (Chip Type = WS2812),
 * * LPD8806 and
 * * APA102 / DotStar.
 *
 * .. versionadded:: 2.0.2$nbsp;(Plugin)
 */
int led_strip_set_chip_type(LEDStrip *led_strip, uint16_t chip);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns the currently used chip type as set by {@link led_strip_set_chip_type}.
 *
 * .. versionadded:: 2.0.2$nbsp;(Plugin)
 */
int led_strip_get_chip_type(LEDStrip *led_strip, uint16_t *ret_chip);

/**
 * \ingroup BrickletLEDStrip
 *
 * Sets *length* RGBW values for the LEDs starting from *index*.
 *
 * To make the colors show correctly you need to configure the chip type
 * ({@link led_strip_set_chip_type}) and a 4-channel channel mapping ({@link led_strip_set_channel_mapping})
 * according to the connected LEDs.
 *
 * The maximum length is 12, the index goes from 0 to 239 and the rgbw values
 * have 8 bits each.
 *
 * Example: If you set
 *
 * * index to 5,
 * * length to 4,
 * * r to [255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
 * * g to [0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
 * * b to [0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0] and
 * * w to [0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0]
 *
 * the LED with index 5 will be red, 6 will be green, 7 will be blue and 8 will be white.
 *
 * \note Depending on the LED circuitry colors can be permuted.
 *
 * The colors will be transfered to actual LEDs when the next
 * frame duration ends, see {@link led_strip_set_frame_duration}.
 *
 * Generic approach:
 *
 * * Set the frame duration to a value that represents
 *   the number of frames per second you want to achieve.
 * * Set all of the LED colors for one frame.
 * * Wait for the {@link LED_STRIP_CALLBACK_FRAME_RENDERED} callback.
 * * Set all of the LED colors for next frame.
 * * Wait for the {@link LED_STRIP_CALLBACK_FRAME_RENDERED} callback.
 * * and so on.
 *
 * This approach ensures that you can change the LED colors with
 * a fixed frame rate.
 *
 * The actual number of controllable LEDs depends on the number of free
 * Bricklet ports. See :ref:`here <led_strip_bricklet_ram_constraints>` for more
 * information. A call of {@link led_strip_set_rgbw_values} with index + length above the
 * bounds is ignored completely.
 *
 * The LPD8806 LED driver chips have 7-bit channels for RGB. Internally the LED
 * Strip Bricklets divides the 8-bit values set using this function by 2 to make
 * them 7-bit. Therefore, you can just use the normal value range (0-255) for
 * LPD8806 LEDs.
 *
 * The brightness channel of the APA102 LED driver chips has 5-bit. Internally the
 * LED Strip Bricklets divides the 8-bit values set using this function by 8 to make
 * them 5-bit. Therefore, you can just use the normal value range (0-255) for
 * the brightness channel of APA102 LEDs.
 *
 * .. versionadded:: 2.0.6$nbsp;(Plugin)
 */
int led_strip_set_rgbw_values(LEDStrip *led_strip, uint16_t index, uint8_t length, uint8_t r[12], uint8_t g[12], uint8_t b[12], uint8_t w[12]);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns *length* RGBW values starting from the given *index*.
 *
 * The values are the last values that were set by {@link led_strip_set_rgbw_values}.
 *
 * .. versionadded:: 2.0.6$nbsp;(Plugin)
 */
int led_strip_get_rgbw_values(LEDStrip *led_strip, uint16_t index, uint8_t length, uint8_t ret_r[12], uint8_t ret_g[12], uint8_t ret_b[12], uint8_t ret_w[12]);

/**
 * \ingroup BrickletLEDStrip
 *
 * Sets the channel mapping for the connected LEDs.
 *
 * {@link led_strip_set_rgb_values} and {@link led_strip_set_rgbw_values} take the data in RGB(W) order.
 * But the connected LED driver chips might have their 3 or 4 channels in a
 * different order. For example, the WS2801 chips typically use BGR order, the
 * WS2812 chips typically use GRB order and the APA102 chips typically use WBGR
 * order.
 *
 * The APA102 chips are special. They have three 8-bit channels for RGB
 * and an additional 5-bit channel for the overall brightness of the RGB LED
 * making them 4-channel chips. Internally the brightness channel is the first
 * channel, therefore one of the Wxyz channel mappings should be used. Then
 * the W channel controls the brightness.
 *
 * If a 3-channel mapping is selected then {@link led_strip_set_rgb_values} has to be used.
 * Calling {@link led_strip_set_rgbw_values} with a 3-channel mapping will produce incorrect
 * results. Vice-versa if a 4-channel mapping is selected then
 * {@link led_strip_set_rgbw_values} has to be used. Calling {@link led_strip_set_rgb_values} with a
 * 4-channel mapping will produce incorrect results.
 *
 * .. versionadded:: 2.0.6$nbsp;(Plugin)
 */
int led_strip_set_channel_mapping(LEDStrip *led_strip, uint8_t mapping);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns the currently used channel mapping as set by {@link led_strip_set_channel_mapping}.
 *
 * .. versionadded:: 2.0.6$nbsp;(Plugin)
 */
int led_strip_get_channel_mapping(LEDStrip *led_strip, uint8_t *ret_mapping);

/**
 * \ingroup BrickletLEDStrip
 *
 * Enables the {@link LED_STRIP_CALLBACK_FRAME_RENDERED} callback.
 *
 * By default the callback is enabled.
 *
 * .. versionadded:: 2.0.6$nbsp;(Plugin)
 */
int led_strip_enable_frame_rendered_callback(LEDStrip *led_strip);

/**
 * \ingroup BrickletLEDStrip
 *
 * Disables the {@link LED_STRIP_CALLBACK_FRAME_RENDERED} callback.
 *
 * By default the callback is enabled.
 *
 * .. versionadded:: 2.0.6$nbsp;(Plugin)
 */
int led_strip_disable_frame_rendered_callback(LEDStrip *led_strip);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns *true* if the {@link LED_STRIP_CALLBACK_FRAME_RENDERED} callback is enabled, *false* otherwise.
 *
 * .. versionadded:: 2.0.6$nbsp;(Plugin)
 */
int led_strip_is_frame_rendered_callback_enabled(LEDStrip *led_strip, bool *ret_enabled);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns the UID, the UID where the Bricklet is connected to,
 * the position, the hardware and firmware version as well as the
 * device identifier.
 *
 * The position can be 'a', 'b', 'c', 'd', 'e', 'f', 'g' or 'h' (Bricklet Port).
 * A Bricklet connected to an :ref:`Isolator Bricklet <isolator_bricklet>` is always at
 * position 'z'.
 *
 * The device identifier numbers can be found :ref:`here <device_identifier>`.
 * |device_identifier_constant|
 */
int led_strip_get_identity(LEDStrip *led_strip, char ret_uid[8], char ret_connected_uid[8], char *ret_position, uint8_t ret_hardware_version[3], uint8_t ret_firmware_version[3], uint16_t *ret_device_identifier);

#ifdef __cplusplus
}
#endif

#endif
