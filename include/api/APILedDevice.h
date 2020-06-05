#pragma once

// led device
#include <leddevice/Rs232Util.h>

namespace APILedDevice {
	/// Represents a device
	struct SerialPortDevice {
		QString portName;
		QString manufacturer;
		QString description;
		QString systemLocation;
	};
	///
    /// @brief Get a list of current system serial devices
    /// @return The result
    ///
    QList<SerialPortDevice> getSerialPorts()
	{
		QList<SerialPortDevice> devices;
		for (const auto &entry : Rs232Util::getSerialPorts())
		{
			devices.append(
				SerialPortDevice{
					entry.portName(),
					entry.manufacturer(),
					entry.description(),
					entry.systemLocation()
				}
			);
		}
		return devices;
	}
}
