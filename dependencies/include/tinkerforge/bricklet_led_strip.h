/* ***********************************************************
 * This file was automatically generated on 2013-12-19.      *
 *                                                           *
 * Bindings Version 2.0.13                                    *
 *                                                           *
 * If you have a bugfix for this file and want to commit it, *
 * please fix the bug in the generator. You can find a link  *
 * to the generator git on tinkerforge.com                   *
 *************************************************************/

#ifndef BRICKLET_LED_STRIP_H
#define BRICKLET_LED_STRIP_H

#include "ip_connection.h"

/**
 * \defgroup BrickletLEDStrip LEDStrip Bricklet
 */

/**
 * \ingroup BrickletLEDStrip
 *
 * Device to control up to 320 RGB LEDs
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
#define LED_STRIP_FUNCTION_GET_IDENTITY 255

/**
 * \ingroup BrickletLEDStrip
 *
 * Signature: \code void callback(uint16_t length, void *user_data) \endcode
 * 
 * This callback is triggered directly after a new frame is rendered.
 * 
 * You should send the data for the next frame directly after this callback
 * was triggered.
 * 
 * For an explanation of the general approach see {@link led_strip_set_rgb_values}.
 */
#define LED_STRIP_CALLBACK_FRAME_RENDERED 6


/**
 * \ingroup BrickletLEDStrip
 *
 * This constant is used to identify a LEDStrip Bricklet.
 *
 * The {@link led_strip_get_identity} function and the
 * {@link IPCON_CALLBACK_ENUMERATE} callback of the IP Connection have a
 * \c device_identifier parameter to specify the Brick's or Bricklet's type.
 */
#define LED_STRIP_DEVICE_IDENTIFIER 231

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
 * disabled for a setter function then no response is send and errors are
 * silently ignored, because they cannot be detected.
 */
int led_strip_get_response_expected(LEDStrip *led_strip, uint8_t function_id, bool *ret_response_expected);

/**
 * \ingroup BrickletLEDStrip
 *
 * Changes the response expected flag of the function specified by the
 * \c function_id parameter. This flag can only be changed for setter
 * (default value: *false*) and callback configuration functions
 * (default value: *true*). For getter functions it is always enabled and
 * callbacks it is always disabled.
 *
 * Enabling the response expected flag for a setter function allows to detect
 * timeouts and other error conditions calls of this setter as well. The device
 * will then send a response for this purpose. If this flag is disabled for a
 * setter function then no response is send and errors are silently ignored,
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
 * Registers a callback with ID \c id to the function \c callback. The
 * \c user_data will be given as a parameter of the callback.
 */
void led_strip_register_callback(LEDStrip *led_strip, uint8_t id, void *callback, void *user_data);

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
 * Sets the *rgb* values for the LEDs with the given *length* starting 
 * from *index*.
 * 
 * The maximum length is 16, the index goes from 0 to 319 and the rgb values
 * have 8 bits each.
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
 * Returns the rgb with the given *length* starting from the
 * given *index*.
 * 
 * The values are the last values that were set by {@link led_strip_set_rgb_values}.
 */
int led_strip_get_rgb_values(LEDStrip *led_strip, uint16_t index, uint8_t length, uint8_t ret_r[16], uint8_t ret_g[16], uint8_t ret_b[16]);

/**
 * \ingroup BrickletLEDStrip
 *
 * Sets the frame duration in ms.
 * 
 * Example: If you want to achieve 20 frames per second, you should
 * set the frame duration to 50ms (50ms * 20 = 1 second). 
 * 
 * For an explanation of the general approach see {@link led_strip_set_rgb_values}.
 * 
 * Default value: 100ms (10 frames per second).
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
 * Returns the current supply voltage of the LEDs. The voltage is given in mV.
 */
int led_strip_get_supply_voltage(LEDStrip *led_strip, uint16_t *ret_voltage);

/**
 * \ingroup BrickletLEDStrip
 *
 * Sets the frequency of the clock in Hz. The range is 10000Hz (10kHz) up to
 * 2000000Hz (2MHz).
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
 * The default value is 1.66MHz.
 * 
 * \note
 *  The frequency in firmware version 2.0.0 is fixed at 2MHz.
 * 
 * .. versionadded:: 2.0.1~(Plugin)
 */
int led_strip_set_clock_frequency(LEDStrip *led_strip, uint32_t frequency);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns the currently used clock frequency.
 * 
 * .. versionadded:: 2.0.1~(Plugin)
 */
int led_strip_get_clock_frequency(LEDStrip *led_strip, uint32_t *ret_frequency);

/**
 * \ingroup BrickletLEDStrip
 *
 * Returns the UID, the UID where the Bricklet is connected to, 
 * the position, the hardware and firmware version as well as the
 * device identifier.
 * 
 * The position can be 'a', 'b', 'c' or 'd'.
 * 
 * The device identifiers can be found :ref:`here <device_identifier>`.
 * 
 * .. versionadded:: 2.0.0~(Plugin)
 */
int led_strip_get_identity(LEDStrip *led_strip, char ret_uid[8], char ret_connected_uid[8], char *ret_position, uint8_t ret_hardware_version[3], uint8_t ret_firmware_version[3], uint16_t *ret_device_identifier);

#endif
