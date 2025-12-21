#include "LedDeviceFile.h"

// Qt includes
#include <Qt>
#include <QTextStream>
#include <QDateTime>

LedDeviceFile::LedDeviceFile(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	, _file (nullptr)
{
	_printTimeStamp = false;
}

LedDevice* LedDeviceFile::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceFile(deviceConfig);
}

bool LedDeviceFile::init(const QJsonObject &deviceConfig)
{
	// Initialise sub-class
	if (!LedDevice::init(deviceConfig))
	{
		return false;
	}

	_fileName = deviceConfig["output"].toString("/dev/null");

#if _WIN32
	if (_fileName == "/dev/null" )
	{
		_fileName = "\\\\.\\NUL";
	}
#endif

	_printTimeStamp = deviceConfig["printTimeStamp"].toBool(false);

	initFile(_fileName);

	return true;
}

void LedDeviceFile::initFile(const QString &fileName)
{
	if ( _file == nullptr )
	{
		_file.reset(new QFile(fileName));
	}
}

int LedDeviceFile::open()
{
	_isDeviceReady = false;

	if ( _file->isOpen() )
	{
		return 0;
	}

	Debug(_log, "QIODevice::WriteOnly, %s", QSTRING_CSTR(_fileName));
	if ( !_file->open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		QString errortext = QString ("(%1) %2, file: (%3)").arg(_file->error()).arg(_file->errorString(),_fileName);
		this->setInError( errortext );
		return -1;
	}

	_isDeviceReady = true;

	return 0;
}

int LedDeviceFile::close()
{
	_isDeviceReady = false;
	if ( _file != nullptr)
	{
		// Test, if device requires closing
		if ( _file->isOpen() )
		{
			// close device physically
			Debug(_log,"File: %s", QSTRING_CSTR(_fileName) );
			_file->close();
		}
	}
	return 0;
}

bool LedDeviceFile::powerOff()
{
	// Simulate power-off by writing a final "Black" to have a defined outcome
	bool rc = false;
	if ( writeBlack( 5 ) >= 0 )
	{
		rc = true;
	}
	return rc;
}

int LedDeviceFile::write(const QVector<ColorRgb> & ledValues)
{
	QTextStream out(_file.get());
	if ( _printTimeStamp )
	{
		QDateTime now = QDateTime::currentDateTime();
		qint64 elapsedTimeMs = _lastWriteTime.msecsTo(now);

		#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
			out << now.toString(Qt::ISODateWithMs) << " | +" << QString("%1").arg( elapsedTimeMs,4);
		#else
			out << now.toString(Qt::ISODate) << now.toString(".zzz") << " | +" << QString("%1").arg( elapsedTimeMs,4);
		#endif
	}

	out << " [";
	for (const ColorRgb& color : ledValues)
	{
		out << color;
	}

	#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
		out << QString("]") << Qt::endl;
	#else
		out << "]" << endl;
	#endif

	return 0;
}
