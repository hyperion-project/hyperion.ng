
#pragma once

// Hyperion leddevice includes
#include "LedRs232Device.h"

class LedDeviceWs2812b : public LedRs232Device
{
public:
	LedDeviceWs2812b();

	virtual int write(const std::vector<ColorRgb> & ledValues);

	virtual int switchOff();

private:

	struct ByteSignal
	{
		uint8_t bit_12;
		uint8_t bit_34;
		uint8_t bit_56;
		uint8_t bit_78;
	};
	std::vector<ByteSignal> _byte2signalTable;

	void fillTable();

	ByteSignal byte2Signal(const uint8_t byte) const;

	uint8_t bits2Signal(const bool bit1, const bool bit2) const;

	std::vector<ByteSignal> _ledBuffer;
};
