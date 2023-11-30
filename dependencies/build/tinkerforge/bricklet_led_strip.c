/* ***********************************************************
 * This file was automatically generated on 2023-11-30.      *
 *                                                           *
 * C/C++ Bindings Version 2.1.33                             *
 *                                                           *
 * If you have a bugfix for this file and want to commit it, *
 * please fix the bug in the generator. You can find a link  *
 * to the generators git repository on tinkerforge.com       *
 *************************************************************/


#define IPCON_EXPOSE_INTERNALS

#include "bricklet_led_strip.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef void (*FrameRendered_CallbackFunction)(uint16_t length, void *user_data);

#if defined _MSC_VER || defined __BORLANDC__
	#pragma pack(push)
	#pragma pack(1)
	#define ATTRIBUTE_PACKED
#elif defined __GNUC__
	#ifdef _WIN32
		// workaround struct packing bug in GCC 4.7 on Windows
		// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52991
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
} ATTRIBUTE_PACKED SetRGBValues_Request;

typedef struct {
	PacketHeader header;
	uint16_t index;
	uint8_t length;
} ATTRIBUTE_PACKED GetRGBValues_Request;

typedef struct {
	PacketHeader header;
	uint8_t r[16];
	uint8_t g[16];
	uint8_t b[16];
} ATTRIBUTE_PACKED GetRGBValues_Response;

typedef struct {
	PacketHeader header;
	uint16_t duration;
} ATTRIBUTE_PACKED SetFrameDuration_Request;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED GetFrameDuration_Request;

typedef struct {
	PacketHeader header;
	uint16_t duration;
} ATTRIBUTE_PACKED GetFrameDuration_Response;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED GetSupplyVoltage_Request;

typedef struct {
	PacketHeader header;
	uint16_t voltage;
} ATTRIBUTE_PACKED GetSupplyVoltage_Response;

typedef struct {
	PacketHeader header;
	uint16_t length;
} ATTRIBUTE_PACKED FrameRendered_Callback;

typedef struct {
	PacketHeader header;
	uint32_t frequency;
} ATTRIBUTE_PACKED SetClockFrequency_Request;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED GetClockFrequency_Request;

typedef struct {
	PacketHeader header;
	uint32_t frequency;
} ATTRIBUTE_PACKED GetClockFrequency_Response;

typedef struct {
	PacketHeader header;
	uint16_t chip;
} ATTRIBUTE_PACKED SetChipType_Request;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED GetChipType_Request;

typedef struct {
	PacketHeader header;
	uint16_t chip;
} ATTRIBUTE_PACKED GetChipType_Response;

typedef struct {
	PacketHeader header;
	uint16_t index;
	uint8_t length;
	uint8_t r[12];
	uint8_t g[12];
	uint8_t b[12];
	uint8_t w[12];
} ATTRIBUTE_PACKED SetRGBWValues_Request;

typedef struct {
	PacketHeader header;
	uint16_t index;
	uint8_t length;
} ATTRIBUTE_PACKED GetRGBWValues_Request;

typedef struct {
	PacketHeader header;
	uint8_t r[12];
	uint8_t g[12];
	uint8_t b[12];
	uint8_t w[12];
} ATTRIBUTE_PACKED GetRGBWValues_Response;

typedef struct {
	PacketHeader header;
	uint8_t mapping;
} ATTRIBUTE_PACKED SetChannelMapping_Request;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED GetChannelMapping_Request;

typedef struct {
	PacketHeader header;
	uint8_t mapping;
} ATTRIBUTE_PACKED GetChannelMapping_Response;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED EnableFrameRenderedCallback_Request;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED DisableFrameRenderedCallback_Request;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED IsFrameRenderedCallbackEnabled_Request;

typedef struct {
	PacketHeader header;
	uint8_t enabled;
} ATTRIBUTE_PACKED IsFrameRenderedCallbackEnabled_Response;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED GetIdentity_Request;

typedef struct {
	PacketHeader header;
	char uid[8];
	char connected_uid[8];
	char position;
	uint8_t hardware_version[3];
	uint8_t firmware_version[3];
	uint16_t device_identifier;
} ATTRIBUTE_PACKED GetIdentity_Response;

#if defined _MSC_VER || defined __BORLANDC__
	#pragma pack(pop)
#endif
#undef ATTRIBUTE_PACKED

static void led_strip_callback_wrapper_frame_rendered(DevicePrivate *device_p, Packet *packet) {
	FrameRendered_CallbackFunction callback_function;
	void *user_data;
	FrameRendered_Callback *callback;

	if (packet->header.length != sizeof(FrameRendered_Callback)) {
		return; // silently ignoring callback with wrong length
	}

	callback_function = (FrameRendered_CallbackFunction)device_p->registered_callbacks[DEVICE_NUM_FUNCTION_IDS + LED_STRIP_CALLBACK_FRAME_RENDERED];
	user_data = device_p->registered_callback_user_data[DEVICE_NUM_FUNCTION_IDS + LED_STRIP_CALLBACK_FRAME_RENDERED];
	callback = (FrameRendered_Callback *)packet;
	(void)callback; // avoid unused variable warning

	if (callback_function == NULL) {
		return;
	}

	callback->length = leconvert_uint16_from(callback->length);

	callback_function(callback->length, user_data);
}

void led_strip_create(LEDStrip *led_strip, const char *uid, IPConnection *ipcon) {
	IPConnectionPrivate *ipcon_p = ipcon->p;
	DevicePrivate *device_p;

	device_create(led_strip, uid, ipcon_p, 2, 0, 3, LED_STRIP_DEVICE_IDENTIFIER);

	device_p = led_strip->p;

	device_p->response_expected[LED_STRIP_FUNCTION_SET_RGB_VALUES] = DEVICE_RESPONSE_EXPECTED_FALSE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_RGB_VALUES] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_SET_FRAME_DURATION] = DEVICE_RESPONSE_EXPECTED_FALSE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_FRAME_DURATION] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_SUPPLY_VOLTAGE] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_SET_CLOCK_FREQUENCY] = DEVICE_RESPONSE_EXPECTED_FALSE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_CLOCK_FREQUENCY] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_SET_CHIP_TYPE] = DEVICE_RESPONSE_EXPECTED_FALSE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_CHIP_TYPE] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_SET_RGBW_VALUES] = DEVICE_RESPONSE_EXPECTED_FALSE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_RGBW_VALUES] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_SET_CHANNEL_MAPPING] = DEVICE_RESPONSE_EXPECTED_FALSE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_CHANNEL_MAPPING] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_ENABLE_FRAME_RENDERED_CALLBACK] = DEVICE_RESPONSE_EXPECTED_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_DISABLE_FRAME_RENDERED_CALLBACK] = DEVICE_RESPONSE_EXPECTED_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_IS_FRAME_RENDERED_CALLBACK_ENABLED] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[LED_STRIP_FUNCTION_GET_IDENTITY] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;

	device_p->callback_wrappers[LED_STRIP_CALLBACK_FRAME_RENDERED] = led_strip_callback_wrapper_frame_rendered;

	ipcon_add_device(ipcon_p, device_p);
}

void led_strip_destroy(LEDStrip *led_strip) {
	device_release(led_strip->p);
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

void led_strip_register_callback(LEDStrip *led_strip, int16_t callback_id, void (*function)(void), void *user_data) {
	device_register_callback(led_strip->p, callback_id, function, user_data);
}

int led_strip_get_api_version(LEDStrip *led_strip, uint8_t ret_api_version[3]) {
	return device_get_api_version(led_strip->p, ret_api_version);
}

int led_strip_set_rgb_values(LEDStrip *led_strip, uint16_t index, uint8_t length, uint8_t r[16], uint8_t g[16], uint8_t b[16]) {
	DevicePrivate *device_p = led_strip->p;
	SetRGBValues_Request request;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_SET_RGB_VALUES, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.index = leconvert_uint16_to(index);
	request.length = length;
	memcpy(request.r, r, 16 * sizeof(uint8_t));
	memcpy(request.g, g, 16 * sizeof(uint8_t));
	memcpy(request.b, b, 16 * sizeof(uint8_t));

	ret = device_send_request(device_p, (Packet *)&request, NULL, 0);

	return ret;
}

int led_strip_get_rgb_values(LEDStrip *led_strip, uint16_t index, uint8_t length, uint8_t ret_r[16], uint8_t ret_g[16], uint8_t ret_b[16]) {
	DevicePrivate *device_p = led_strip->p;
	GetRGBValues_Request request;
	GetRGBValues_Response response;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_RGB_VALUES, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.index = leconvert_uint16_to(index);
	request.length = length;

	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response, sizeof(response));

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
	SetFrameDuration_Request request;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_SET_FRAME_DURATION, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.duration = leconvert_uint16_to(duration);

	ret = device_send_request(device_p, (Packet *)&request, NULL, 0);

	return ret;
}

int led_strip_get_frame_duration(LEDStrip *led_strip, uint16_t *ret_duration) {
	DevicePrivate *device_p = led_strip->p;
	GetFrameDuration_Request request;
	GetFrameDuration_Response response;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_FRAME_DURATION, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response, sizeof(response));

	if (ret < 0) {
		return ret;
	}

	*ret_duration = leconvert_uint16_from(response.duration);

	return ret;
}

int led_strip_get_supply_voltage(LEDStrip *led_strip, uint16_t *ret_voltage) {
	DevicePrivate *device_p = led_strip->p;
	GetSupplyVoltage_Request request;
	GetSupplyVoltage_Response response;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_SUPPLY_VOLTAGE, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response, sizeof(response));

	if (ret < 0) {
		return ret;
	}

	*ret_voltage = leconvert_uint16_from(response.voltage);

	return ret;
}

int led_strip_set_clock_frequency(LEDStrip *led_strip, uint32_t frequency) {
	DevicePrivate *device_p = led_strip->p;
	SetClockFrequency_Request request;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_SET_CLOCK_FREQUENCY, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.frequency = leconvert_uint32_to(frequency);

	ret = device_send_request(device_p, (Packet *)&request, NULL, 0);

	return ret;
}

int led_strip_get_clock_frequency(LEDStrip *led_strip, uint32_t *ret_frequency) {
	DevicePrivate *device_p = led_strip->p;
	GetClockFrequency_Request request;
	GetClockFrequency_Response response;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_CLOCK_FREQUENCY, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response, sizeof(response));

	if (ret < 0) {
		return ret;
	}

	*ret_frequency = leconvert_uint32_from(response.frequency);

	return ret;
}

int led_strip_set_chip_type(LEDStrip *led_strip, uint16_t chip) {
	DevicePrivate *device_p = led_strip->p;
	SetChipType_Request request;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_SET_CHIP_TYPE, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.chip = leconvert_uint16_to(chip);

	ret = device_send_request(device_p, (Packet *)&request, NULL, 0);

	return ret;
}

int led_strip_get_chip_type(LEDStrip *led_strip, uint16_t *ret_chip) {
	DevicePrivate *device_p = led_strip->p;
	GetChipType_Request request;
	GetChipType_Response response;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_CHIP_TYPE, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response, sizeof(response));

	if (ret < 0) {
		return ret;
	}

	*ret_chip = leconvert_uint16_from(response.chip);

	return ret;
}

int led_strip_set_rgbw_values(LEDStrip *led_strip, uint16_t index, uint8_t length, uint8_t r[12], uint8_t g[12], uint8_t b[12], uint8_t w[12]) {
	DevicePrivate *device_p = led_strip->p;
	SetRGBWValues_Request request;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_SET_RGBW_VALUES, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.index = leconvert_uint16_to(index);
	request.length = length;
	memcpy(request.r, r, 12 * sizeof(uint8_t));
	memcpy(request.g, g, 12 * sizeof(uint8_t));
	memcpy(request.b, b, 12 * sizeof(uint8_t));
	memcpy(request.w, w, 12 * sizeof(uint8_t));

	ret = device_send_request(device_p, (Packet *)&request, NULL, 0);

	return ret;
}

int led_strip_get_rgbw_values(LEDStrip *led_strip, uint16_t index, uint8_t length, uint8_t ret_r[12], uint8_t ret_g[12], uint8_t ret_b[12], uint8_t ret_w[12]) {
	DevicePrivate *device_p = led_strip->p;
	GetRGBWValues_Request request;
	GetRGBWValues_Response response;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_RGBW_VALUES, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.index = leconvert_uint16_to(index);
	request.length = length;

	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response, sizeof(response));

	if (ret < 0) {
		return ret;
	}

	memcpy(ret_r, response.r, 12 * sizeof(uint8_t));
	memcpy(ret_g, response.g, 12 * sizeof(uint8_t));
	memcpy(ret_b, response.b, 12 * sizeof(uint8_t));
	memcpy(ret_w, response.w, 12 * sizeof(uint8_t));

	return ret;
}

int led_strip_set_channel_mapping(LEDStrip *led_strip, uint8_t mapping) {
	DevicePrivate *device_p = led_strip->p;
	SetChannelMapping_Request request;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_SET_CHANNEL_MAPPING, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	request.mapping = mapping;

	ret = device_send_request(device_p, (Packet *)&request, NULL, 0);

	return ret;
}

int led_strip_get_channel_mapping(LEDStrip *led_strip, uint8_t *ret_mapping) {
	DevicePrivate *device_p = led_strip->p;
	GetChannelMapping_Request request;
	GetChannelMapping_Response response;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_CHANNEL_MAPPING, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response, sizeof(response));

	if (ret < 0) {
		return ret;
	}

	*ret_mapping = response.mapping;

	return ret;
}

int led_strip_enable_frame_rendered_callback(LEDStrip *led_strip) {
	DevicePrivate *device_p = led_strip->p;
	EnableFrameRenderedCallback_Request request;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_ENABLE_FRAME_RENDERED_CALLBACK, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	ret = device_send_request(device_p, (Packet *)&request, NULL, 0);

	return ret;
}

int led_strip_disable_frame_rendered_callback(LEDStrip *led_strip) {
	DevicePrivate *device_p = led_strip->p;
	DisableFrameRenderedCallback_Request request;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_DISABLE_FRAME_RENDERED_CALLBACK, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	ret = device_send_request(device_p, (Packet *)&request, NULL, 0);

	return ret;
}

int led_strip_is_frame_rendered_callback_enabled(LEDStrip *led_strip, bool *ret_enabled) {
	DevicePrivate *device_p = led_strip->p;
	IsFrameRenderedCallbackEnabled_Request request;
	IsFrameRenderedCallbackEnabled_Response response;
	int ret;

	ret = device_check_validity(device_p);

	if (ret < 0) {
		return ret;
	}

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_IS_FRAME_RENDERED_CALLBACK_ENABLED, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response, sizeof(response));

	if (ret < 0) {
		return ret;
	}

	*ret_enabled = response.enabled != 0;

	return ret;
}

int led_strip_get_identity(LEDStrip *led_strip, char ret_uid[8], char ret_connected_uid[8], char *ret_position, uint8_t ret_hardware_version[3], uint8_t ret_firmware_version[3], uint16_t *ret_device_identifier) {
	DevicePrivate *device_p = led_strip->p;
	GetIdentity_Request request;
	GetIdentity_Response response;
	int ret;

	ret = packet_header_create(&request.header, sizeof(request), LED_STRIP_FUNCTION_GET_IDENTITY, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response, sizeof(response));

	if (ret < 0) {
		return ret;
	}

	memcpy(ret_uid, response.uid, 8);
	memcpy(ret_connected_uid, response.connected_uid, 8);
	*ret_position = response.position;
	memcpy(ret_hardware_version, response.hardware_version, 3 * sizeof(uint8_t));
	memcpy(ret_firmware_version, response.firmware_version, 3 * sizeof(uint8_t));
	*ret_device_identifier = leconvert_uint16_from(response.device_identifier);

	return ret;
}

#ifdef __cplusplus
}
#endif
