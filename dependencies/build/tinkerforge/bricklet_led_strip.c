/* ***********************************************************
 * This file was automatically generated on 2013-12-19.      *
 *                                                           *
 * Bindings Version 2.0.13                                    *
 *                                                           *
 * If you have a bugfix for this file and want to commit it, *
 * please fix the bug in the generator. You can find a link  *
 * to the generator git on tinkerforge.com                   *
 *************************************************************/


#define IPCON_EXPOSE_INTERNALS

#include "bricklet_led_strip.h"

#include <string.h>



typedef void (*FrameRenderedCallbackFunction)(uint16_t, void *);

#if defined _MSC_VER || defined __BORLANDC__
	#pragma pack(push)
	#pragma pack(1)
	#define ATTRIBUTE_PACKED
#elif defined __GNUC__
	#ifdef _WIN32
		// workaround struct packing bug in GCC 4.7 on Windows
		// http://gcc.gnu.org/bugzilla/show_bug.cgi?id=52991
		#define ATTRIBUTE_PACKED __attribute__((gcc_struct, packed))
	#else
		#define ATTRIBUTE_PACKED __attribute__((packed))
	#endif
#else
	#error unknown compiler, do not know how to enable struct packing
#endif

typedef struct {
	PacketHeader header;
	uint16_t index;
	uint8_t length;
	uint8_t r[16];
	uint8_t g[16];
	uint8_t b[16];
} ATTRIBUTE_PACKED SetRGBValues_;

typedef struct {
	PacketHeader header;
	uint16_t index;
	uint8_t length;
} ATTRIBUTE_PACKED GetRGBValues_;

typedef struct {
	PacketHeader header;
	uint8_t r[16];
	uint8_t g[16];
	uint8_t b[16];
} ATTRIBUTE_PACKED GetRGBValuesResponse_;

typedef struct {
	PacketHeader header;
	uint16_t duration;
} ATTRIBUTE_PACKED SetFrameDuration_;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED GetFrameDuration_;

typedef struct {
	PacketHeader header;
	uint16_t duration;
} ATTRIBUTE_PACKED GetFrameDurationResponse_;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED GetSupplyVoltage_;

typedef struct {
	PacketHeader header;
	uint16_t voltage;
} ATTRIBUTE_PACKED GetSupplyVoltageResponse_;

typedef struct {
	PacketHeader header;
	uint16_t length;
} ATTRIBUTE_PACKED FrameRenderedCallback_;

typedef struct {
	PacketHeader header;
	uint32_t frequency;
} ATTRIBUTE_PACKED SetClockFrequency_;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED GetClockFrequency_;

typedef struct {
	PacketHeader header;
	uint32_t frequency;
} ATTRIBUTE_PACKED GetClockFrequencyResponse_;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED GetIdentity_;

typedef struct {
	PacketHeader header;
	char uid[8];
	char connected_uid[8];
	char position;
	uint8_t hardware_version[3];
	uint8_t firmware_version[3];
	uint16_t device_identifier;
} ATTRIBUTE_PACKED GetIdentityResponse_;

#if defined _MSC_VER || defined __BORLANDC__
	#pragma pack(pop)
#endif
#undef ATTRIBUTE_PACKED

static void led_strip_callback_wrapper_frame_rendered(DevicePrivate *device_p, Packet *packet) {
	FrameRenderedCallbackFunction callback_function;
	void *user_data = device_p->registered_callback_user_data[LED_STRIP_CALLBACK_FRAME_RENDERED];
	FrameRenderedCallback_ *callback = (FrameRenderedCallback_ *)packet;
	*(void **)(&callback_function) = device_p->registered_callbacks[LED_STRIP_CALLBACK_FRAME_RENDERED];

	if (callback_function == NULL) {
		return;
	}

	callback->length = leconvert_uint16_from(callback->length);

	callback_function(callback->length, user_data);
}

void led_strip_create(LEDStrip *led_strip, const char *uid, IPConnection *ipcon) {
	DevicePrivate *device_p;

	device_create(led_strip, uid, ipcon->p, 2, 0, 1);

	device_p = led_strip->p;

	device_p->response_expected[LED_STRIP_FUNCTION_SET_RGB_VALUES] = DEVICE_RESPONSE_EXPECTED_FALSE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_RGB_VALUES] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_SET_FRAME_DURATION] = DEVICE_RESPONSE_EXPECTED_FALSE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_FRAME_DURATION] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_SUPPLY_VOLTAGE] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_CALLBACK_FRAME_RENDERED] = DEVICE_RESPONSE_EXPECTED_ALWAYS_FALSE;
	device_p->response_expected[LED_STRIP_FUNCTION_SET_CLOCK_FREQUENCY] = DEVICE_RESPONSE_EXPECTED_FALSE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_CLOCK_FREQUENCY] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_IDENTITY] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;

	device_p->callback_wrappers[LED_STRIP_CALLBACK_FRAME_RENDERED] = led_strip_callback_wrapper_frame_rendered;
}

void led_strip_destroy(LEDStrip *led_strip) {
	device_destroy(led_strip);
}

int led_strip_get_response_expected(LEDStrip *led_strip, uint8_t function_id, bool *ret_response_expected) {
	return device_get_response_expected(led_strip->p, function_id, ret_response_expected);
}

int led_strip_set_response_expected(LEDStrip *led_strip, uint8_t function_id, bool response_expected) {
	return device_set_response_expected(led_strip->p, function_id, response_expected);
}

int led_strip_set_response_expected_all(LEDStrip *led_strip, bool response_expected) {
	return device_set_response_expected_all(led_strip->p, response_expected);
}

void led_strip_register_callback(LEDStrip *led_strip, uint8_t id, void *callback, void *user_data) {
	device_register_callback(led_strip->p, id, callback, user_data);
}

int led_strip_get_api_version(LEDStrip *led_strip, uint8_t ret_api_version[3]) {
	return device_get_api_version(led_strip->p, ret_api_version);
}

int led_strip_set_rgb_values(LEDStrip *led_strip, uint16_t index, uint8_t length, uint8_t r[16], uint8_t g[16], uint8_t b[16]) {
	DevicePrivate *device_p = led_strip->p;
	SetRGBValues_ request;
	int ret;

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_SET_RGB_VALUES, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.index = leconvert_uint16_to(index);
	request.length = length;
	memcpy(request.r, r, 16 * sizeof(uint8_t));
	memcpy(request.g, g, 16 * sizeof(uint8_t));
	memcpy(request.b, b, 16 * sizeof(uint8_t));

	ret = device_send_request(device_p, (Packet *)&request, NULL);


	return ret;
}

int led_strip_get_rgb_values(LEDStrip *led_strip, uint16_t index, uint8_t length, uint8_t ret_r[16], uint8_t ret_g[16], uint8_t ret_b[16]) {
	DevicePrivate *device_p = led_strip->p;
	GetRGBValues_ request;
	GetRGBValuesResponse_ response;
	int ret;

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_RGB_VALUES, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.index = leconvert_uint16_to(index);
	request.length = length;

	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response);

	if (ret < 0) {
		return ret;
	}
	memcpy(ret_r, response.r, 16 * sizeof(uint8_t));
	memcpy(ret_g, response.g, 16 * sizeof(uint8_t));
	memcpy(ret_b, response.b, 16 * sizeof(uint8_t));



	return ret;
}

int led_strip_set_frame_duration(LEDStrip *led_strip, uint16_t duration) {
	DevicePrivate *device_p = led_strip->p;
	SetFrameDuration_ request;
	int ret;

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_SET_FRAME_DURATION, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.duration = leconvert_uint16_to(duration);

	ret = device_send_request(device_p, (Packet *)&request, NULL);


	return ret;
}

int led_strip_get_frame_duration(LEDStrip *led_strip, uint16_t *ret_duration) {
	DevicePrivate *device_p = led_strip->p;
	GetFrameDuration_ request;
	GetFrameDurationResponse_ response;
	int ret;

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_FRAME_DURATION, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}


	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response);

	if (ret < 0) {
		return ret;
	}
	*ret_duration = leconvert_uint16_from(response.duration);



	return ret;
}

int led_strip_get_supply_voltage(LEDStrip *led_strip, uint16_t *ret_voltage) {
	DevicePrivate *device_p = led_strip->p;
	GetSupplyVoltage_ request;
	GetSupplyVoltageResponse_ response;
	int ret;

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_SUPPLY_VOLTAGE, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}


	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response);

	if (ret < 0) {
		return ret;
	}
	*ret_voltage = leconvert_uint16_from(response.voltage);



	return ret;
}

int led_strip_set_clock_frequency(LEDStrip *led_strip, uint32_t frequency) {
	DevicePrivate *device_p = led_strip->p;
	SetClockFrequency_ request;
	int ret;

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_SET_CLOCK_FREQUENCY, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.frequency = leconvert_uint32_to(frequency);

	ret = device_send_request(device_p, (Packet *)&request, NULL);


	return ret;
}

int led_strip_get_clock_frequency(LEDStrip *led_strip, uint32_t *ret_frequency) {
	DevicePrivate *device_p = led_strip->p;
	GetClockFrequency_ request;
	GetClockFrequencyResponse_ response;
	int ret;

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_CLOCK_FREQUENCY, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}


	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response);

	if (ret < 0) {
		return ret;
	}
	*ret_frequency = leconvert_uint32_from(response.frequency);



	return ret;
}

int led_strip_get_identity(LEDStrip *led_strip, char ret_uid[8], char ret_connected_uid[8], char *ret_position, uint8_t ret_hardware_version[3], uint8_t ret_firmware_version[3], uint16_t *ret_device_identifier) {
	DevicePrivate *device_p = led_strip->p;
	GetIdentity_ request;
	GetIdentityResponse_ response;
	int ret;

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_IDENTITY, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}


	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response);

	if (ret < 0) {
		return ret;
	}
	strncpy(ret_uid, response.uid, 8);
	strncpy(ret_connected_uid, response.connected_uid, 8);
	*ret_position = response.position;
	memcpy(ret_hardware_version, response.hardware_version, 3 * sizeof(uint8_t));
	memcpy(ret_firmware_version, response.firmware_version, 3 * sizeof(uint8_t));
	*ret_device_identifier = leconvert_uint16_from(response.device_identifier);



	return ret;
}
