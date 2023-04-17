#pragma once
#include <ftdi.h>
#include "providers/BaseProvider.h"

class ProviderFtdi : public BaseProvider
{
public:
	///
	/// Constructs specific LedDevice
	///
    ProviderFtdi(const QJsonObject& deviceConfig);
    ~ProviderFtdi();

	///
	/// Opens and configures the output device
	///
	/// @return Zero on success else negative
	///
	int open() override ;

    QJsonArray discover(const QJsonObject& params) override;

    int writeBytes(unsigned size, const uint8_t *data) override ;

	int close() override ;

protected:
    /// The Ftdi serial-device
	struct ftdi_context *_ftdic;
};
