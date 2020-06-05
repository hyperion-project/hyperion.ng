
#include <QSerialPortInfo>

namespace Rs232Util{
	///
	/// Provide a list of available serial ports of the System
	///
	QList<QSerialPortInfo> getSerialPorts()
	{
		QList<QSerialPortInfo> targets;
		for( auto port : QSerialPortInfo::availablePorts()){
			if (port.hasProductIdentifier() && port.hasVendorIdentifier()){
				targets.append(port);
			}
		}
		return targets;
	}
}
