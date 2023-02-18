#include <grabber/AudioGrabberWindows.h>
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>

#pragma comment(lib,"dsound.lib")
#pragma comment(lib, "dxguid.lib")

// Constants
namespace {
	const int AUDIO_NOTIFICATION_COUNT{ 4 };
} //End of constants

AudioGrabberWindows::AudioGrabberWindows() : AudioGrabber()
{
}

AudioGrabberWindows::~AudioGrabberWindows()
{
	this->stop();
}

void AudioGrabberWindows::refreshDevices()
{
	Debug(_log, "Refreshing Audio Devices");

	_deviceProperties.clear();

	// Enumerate Devices
	if (FAILED(DirectSoundCaptureEnumerate(DirectSoundEnumProcessor, (VOID*)&_deviceProperties)))
	{
		Error(_log, "Failed to enumerate audio devices.");
	}
}

bool AudioGrabberWindows::configureCaptureInterface()
{
	CLSID deviceId {};

	if (!this->_device.isEmpty() && this->_device != "auto")
	{
		LPCOLESTR clsid = reinterpret_cast<const wchar_t*>(_device.utf16());
		HRESULT res = CLSIDFromString(clsid, &deviceId);
		if (FAILED(res))
		{
			Error(_log, "Failed to get CLSID for '%s' with error: 0x%08x: %s", QSTRING_CSTR(_device), res, std::system_category().message(res).c_str());
			return false;
		}
	}

	// Create Capture Device
	HRESULT res = DirectSoundCaptureCreate8(&deviceId, &recordingDevice, NULL);
	if (FAILED(res))
	{
		Error(_log, "Failed to create capture device: '%s' with error: 0x%08x: %s", QSTRING_CSTR(_device), res, std::system_category().message(res).c_str());
		return false;
	}

	// Define Audio Format & Create Buffer
	WAVEFORMATEX audioFormat { WAVE_FORMAT_PCM, 1, 44100, 88200, 2, 16, 0 };
	// wFormatTag, nChannels, nSamplesPerSec, mAvgBytesPerSec,
	// nBlockAlign, wBitsPerSample, cbSize

	notificationSize = max(1024, audioFormat.nAvgBytesPerSec / 8);
	notificationSize -= notificationSize % audioFormat.nBlockAlign;

	bufferCaptureSize = notificationSize * AUDIO_NOTIFICATION_COUNT;
			
	DSCBUFFERDESC bufferDesc;
	bufferDesc.dwSize = sizeof(DSCBUFFERDESC);
	bufferDesc.dwFlags = 0;
	bufferDesc.dwBufferBytes = bufferCaptureSize;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = &audioFormat;
	bufferDesc.dwFXCount = 0;
	bufferDesc.lpDSCFXDesc = NULL;
				
	// Create Capture Device's Buffer
	LPDIRECTSOUNDCAPTUREBUFFER preBuffer;
	if (FAILED(recordingDevice->CreateCaptureBuffer(&bufferDesc, &preBuffer, NULL)))
	{
		Error(_log, "Failed to create capture buffer: '%s'", QSTRING_CSTR(getDeviceName(_device)));
		recordingDevice->Release();
		return false;
	}

	bufferCapturePosition = 0;

	// Query Capture8 Buffer
	if (FAILED(preBuffer->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&recordingBuffer)))
	{
		Error(_log, "Failed to retrieve recording buffer");
		preBuffer->Release();
		return false;
	}

	preBuffer->Release();
		
	// Create Notifications
	LPDIRECTSOUNDNOTIFY8 notify;

	if (FAILED(recordingBuffer->QueryInterface(IID_IDirectSoundNotify8, (LPVOID *) &notify)))
	{
		Error(_log, "Failed to configure buffer notifications: '%s'", QSTRING_CSTR(getDeviceName(_device)));
		recordingDevice->Release();
		recordingBuffer->Release();
		return false;
	}
				
	// Create Events
	notificationEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (notificationEvent == NULL)
	{
		Error(_log, "Failed to configure buffer notifications events: '%s'", QSTRING_CSTR(getDeviceName(_device)));
		notify->Release();
		recordingDevice->Release();
		recordingBuffer->Release();
		return false;
	}

	// Configure Notifications
	DSBPOSITIONNOTIFY positionNotify[AUDIO_NOTIFICATION_COUNT];

	for (int i = 0; i < AUDIO_NOTIFICATION_COUNT; i++)
	{
		positionNotify[i].dwOffset = (notificationSize * i) + notificationSize - 1;
		positionNotify[i].hEventNotify = notificationEvent;
	}
	
	// Set Notifications
	notify->SetNotificationPositions(AUDIO_NOTIFICATION_COUNT, positionNotify);
	notify->Release();
		
	return true;
}

bool AudioGrabberWindows::start()
{
	if (!_isEnabled)
	{
		return false;
	}

	if (this->isRunning.load(std::memory_order_acquire))
	{
		return true;
	}

	//Test, if configured device currently exists
	refreshDevices();
	if (!_deviceProperties.contains(_device))
	{
		_device = "auto";
		Warning(_log, "Configured audio device is not available. Using '%s'", QSTRING_CSTR(getDeviceName(_device)));
	}

	Info(_log, "Capture audio from %s", QSTRING_CSTR(getDeviceName(_device)));
	
	if (!this->configureCaptureInterface())
	{
		return false;
	}
		
	if (FAILED(recordingBuffer->Start(DSCBSTART_LOOPING)))
	{
		Error(_log, "Failed starting audio capture from '%s'", QSTRING_CSTR(getDeviceName(_device)));
		return false;
	}

	this->isRunning.store(true, std::memory_order_release);
	DWORD threadId;

	this->audioThread = CreateThread(
		NULL,
		16,
		AudioThreadRunner,
		(void *) this,
		0,
		&threadId
	);

	if (this->audioThread == NULL)
	{
		Error(_log, "Failed to create audio capture thread");

		this->stop();
		return false;
	}

	AudioGrabber::start();

	return true;
}

void AudioGrabberWindows::stop()
{
	if (!this->isRunning.load(std::memory_order_acquire))
	{
		return;
	}

	Info(_log, "Shutting down audio capture from: '%s'", QSTRING_CSTR(getDeviceName(_device)));

	this->isRunning.store(false, std::memory_order_release);

	if (FAILED(recordingBuffer->Stop()))
	{
		Error(_log, "Audio capture failed to stop: '%s'", QSTRING_CSTR(getDeviceName(_device)));
	}
		
	if (FAILED(recordingBuffer->Release()))
	{
		Error(_log, "Failed to release recording buffer: '%s'", QSTRING_CSTR(getDeviceName(_device)));
	}

	if (FAILED(recordingDevice->Release()))
	{
		Error(_log, "Failed to release recording device: '%s'", QSTRING_CSTR(getDeviceName(_device)));
	}

	CloseHandle(notificationEvent);
	CloseHandle(this->audioThread);

	AudioGrabber::stop();
}

DWORD WINAPI AudioGrabberWindows::AudioThreadRunner(LPVOID param)
{
	AudioGrabberWindows* This = (AudioGrabberWindows*) param;

	while (This->isRunning.load(std::memory_order_acquire))
	{
		DWORD result = WaitForMultipleObjects(1, &This->notificationEvent, true, 500);

		switch (result)
		{
			case WAIT_OBJECT_0:
				This->processAudioBuffer();
				break;
		}
	}

	Debug(This->_log, "Audio capture thread stopped.");

	return 0;
}

void AudioGrabberWindows::processAudioBuffer()
{
	DWORD readPosition;
	DWORD capturePosition;

	// Primary segment
	VOID* capturedAudio;
	DWORD capturedAudioLength;

	// Wrap around segment
	VOID* capturedAudio2;
	DWORD capturedAudio2Length;

	LONG lockSize;

	if (FAILED(recordingBuffer->GetCurrentPosition(&capturePosition, &readPosition)))
	{
		// Failed to get current position
		Error(_log, "Failed to get buffer position.");
		return;
	}

	lockSize = readPosition - bufferCapturePosition;

	if (lockSize < 0)
	{
		lockSize += bufferCaptureSize;
	}

	// Block Align
	lockSize -= (lockSize % notificationSize);

	if (lockSize == 0)
	{
		return;
	}

	// Lock Capture Buffer
	if (FAILED(recordingBuffer->Lock(bufferCapturePosition, lockSize, &capturedAudio, &capturedAudioLength,
		&capturedAudio2, &capturedAudio2Length, 0)))
	{
		// Handle Lock Error
		return;
	}

	bufferCapturePosition += capturedAudioLength;
	bufferCapturePosition %= bufferCaptureSize; // Circular Buffer

	int frameSize = capturedAudioLength + capturedAudio2Length;

	int16_t * readBuffer = new int16_t[frameSize];

	// Buffer wrapped around, read second position
	if (capturedAudio2 != NULL)
	{		
		bufferCapturePosition += capturedAudio2Length;
		bufferCapturePosition %= bufferCaptureSize; // Circular Buffer
	}

	// Copy Buffer into memory
	CopyMemory(readBuffer, capturedAudio, capturedAudioLength);

	if (capturedAudio2 != NULL)
	{
		CopyMemory(readBuffer + capturedAudioLength, capturedAudio2, capturedAudio2Length);
	}
			
	// Release Buffer Lock
	recordingBuffer->Unlock(capturedAudio, capturedAudioLength, capturedAudio2, capturedAudio2Length);
	
	// Process Audio Frame
	this->processAudioFrame(readBuffer, frameSize);
		
	delete[] readBuffer;
}

QJsonArray AudioGrabberWindows::discover(const QJsonObject& params)
{
	refreshDevices();

	QJsonArray devices;

	for (auto deviceIterator = _deviceProperties.begin(); deviceIterator != _deviceProperties.end(); ++deviceIterator)
	{
		// Device
		QJsonObject device;
		QJsonArray deviceInputs;

		device["device"] = deviceIterator.value().id;
		device["device_name"] = deviceIterator.value().name;
		device["type"] = "audio";

		devices.append(device);
	}

	return devices;
}

QString AudioGrabberWindows::getDeviceName(const QString& devicePath) const
{
	if (devicePath.isEmpty() || devicePath == "auto")
	{
		return "Default Device";
	}
	return _deviceProperties.value(devicePath).name;
}
