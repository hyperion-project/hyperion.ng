#ifndef AUDIOGRABBERWINDOWS_H
#define AUDIOGRABBERWINDOWS_H

// Hyperion-utils includes
#include <grabber/audio/AudioGrabber.h>
#include <DSound.h>

///
/// @brief The Windows Audio capture implementation
///
class AudioGrabberWindows : public AudioGrabber
{
	public:

		AudioGrabberWindows();
		~AudioGrabberWindows() override;

	public slots:
		bool start() override;
		void stop() override;
		QJsonArray discover(const QJsonObject& params) override;

	private:
		void refreshDevices();
		bool configureCaptureInterface();
		QString getDeviceName(const QString& devicePath) const;

		void processAudioBuffer();

		LPDIRECTSOUNDCAPTURE8 recordingDevice;
		LPDIRECTSOUNDCAPTUREBUFFER8 recordingBuffer;

		HANDLE audioThread;
		DWORD bufferCapturePosition;
		DWORD bufferCaptureSize;
		DWORD notificationSize;

		static DWORD WINAPI AudioThreadRunner(LPVOID param);
		HANDLE notificationEvent;
		std::atomic<bool> isRunning{ false };

static BOOL CALLBACK DirectSoundEnumProcessor(LPGUID deviceIdGuid, LPCWSTR deviceDescStr,
	LPCWSTR deviceModelStr, LPVOID context)
{
	// Skip undefined audio devices
	if (deviceIdGuid == NULL)
		return TRUE;

	QMap<QString, AudioGrabber::DeviceProperties>* devices = (QMap<QString, AudioGrabber::DeviceProperties>*)context;

	AudioGrabber::DeviceProperties device;

	// Process Device Information
	QString deviceName = QString::fromWCharArray(deviceDescStr);

	// Process Device ID
	LPOLESTR deviceIdStr;
	HRESULT res = StringFromCLSID(*deviceIdGuid, &deviceIdStr);
	if (FAILED(res))
	{
		Error(Logger::getInstance("AUDIOGRABBER"), "Failed to get CLSID-string for %s with error: 0x%08x: %s", QSTRING_CSTR(deviceName), res, std::system_category().message(res).c_str());
		return FALSE;
	}

	QString deviceId = QString::fromWCharArray(deviceIdStr);

	CoTaskMemFree(deviceIdStr);

	Debug(Logger::getInstance("AUDIOGRABBER"), "Found Audio Device: %s", QSTRING_CSTR(deviceName));

	device.id = deviceId;
	device.name = deviceName;

	devices->insert(deviceId, device);

	return TRUE;
}

};

#endif // AUDIOGRABBERWINDOWS_H
