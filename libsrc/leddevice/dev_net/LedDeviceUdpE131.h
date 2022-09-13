#ifndef LEDEVICEUDPE131_H
#define LEDEVICEUDPE131_H

// hyperion includes
#include "ProviderUdp.h"

#include <QUuid>

/**
 *
 * https://raw.githubusercontent.com/forkineye/ESPixelStick/master/_E131.h
 * Project: E131 - E.131 (sACN) library for Arduino
 * Copyright (c) 2015 Shelby Merrick
 * http://www.forkineye.com
 *
 *  This program is provided free for you to use in any way that you wish,
 *  subject to the laws and regulations where you are using it.  Due diligence
 *  is strongly suggested before using this code.  Please give credit where due.
 *
 **/

/* E1.31 Packet Offsets */
//#define E131_ROOT_PREAMBLE_SIZE 0
//#define E131_ROOT_POSTAMBLE_SIZE 2
//#define E131_ROOT_ID 4
//#define E131_ROOT_FLENGTH 16
//#define E131_ROOT_VECTOR 18
//#define E131_ROOT_CID 22

//#define E131_FRAME_FLENGTH 38
//#define E131_FRAME_VECTOR 40
//#define E131_FRAME_SOURCE 44
//#define E131_FRAME_PRIORITY 108
//#define E131_FRAME_RESERVED 109
//#define E131_FRAME_SEQ 111
//#define E131_FRAME_OPT 112
//#define E131_FRAME_UNIVERSE 113

//#define E131_DMP_FLENGTH 115
//#define E131_DMP_VECTOR 117
//#define E131_DMP_TYPE 118
//#define E131_DMP_ADDR_FIRST 119
//#define E131_DMP_ADDR_INC 121
//#define E131_DMP_COUNT 123
const unsigned int E131_DMP_DATA=125;

/* E1.31 Packet Structure */
typedef union
{
#pragma pack(push, 1)
	struct
	{
		/* Root Layer */
		uint16_t preamble_size;
		uint16_t postamble_size;
		uint8_t  acn_id[12];
		uint16_t root_flength;
		uint32_t root_vector;
		char     cid[16];

		/* Frame Layer */
		uint16_t frame_flength;
		uint32_t frame_vector;
		char     source_name[64];
		uint8_t  priority;
		uint16_t reserved;
		uint8_t  sequence_number;
		uint8_t  options;
		uint16_t universe;

		/* DMP Layer */
		uint16_t dmp_flength;
		uint8_t  dmp_vector;
		uint8_t  type;
		uint16_t first_address;
		uint16_t address_increment;
		uint16_t property_value_count;
		uint8_t  property_values[513];
	};
#pragma pack(pop)

	uint8_t raw[638];
} e131_packet_t;

///
/// Implementation of the LedDevice interface for sending led colors via udp/E1.31 packets
///
class LedDeviceUdpE131 : public ProviderUdp
{
public:

	///
	/// @brief Constructs an E1.31 LED-device fed via UDP
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceUdpE131(const QJsonObject &deviceConfig);

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
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;

	///
	/// @brief Generate E1.31 communication header
	///
	void prepare(unsigned this_universe, unsigned this_dmxChannelCount);

	e131_packet_t e131_packet;
	uint8_t _e131_seq = 0;
	uint8_t _e131_universe = 1;
	uint8_t _acn_id[12] = {0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00 };
	QString _e131_source_name;
	QUuid _e131_cid;
};

#endif // LEDEVICEUDPE131_H
