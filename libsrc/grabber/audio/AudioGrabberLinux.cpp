#include <grabber/AudioGrabberLinux.h>

#include <alsa/asoundlib.h>

#include <QJsonObject>
#include <QJsonArray>

typedef void* (*THREADFUNCPTR)(void*);

AudioGrabberLinux::AudioGrabberLinux()
	: AudioGrabber()
	, _isRunning{ false }
	, _captureDevice {nullptr}
	, _sampleRate(44100)
{
}

AudioGrabberLinux::~AudioGrabberLinux()
{
	this->stop();
}

void AudioGrabberLinux::refreshDevices()
{
	Debug(_log, "Enumerating Audio Input Devices");

	_deviceProperties.clear();

	snd_ctl_t* deviceHandle;
	int soundCard {-1};
	int error {-1};
	int cardInput {-1};

	snd_ctl_card_info_t* cardInfo;
	snd_pcm_info_t* deviceInfo;

	snd_ctl_card_info_alloca(&cardInfo);
	snd_pcm_info_alloca(&deviceInfo);

	while (snd_card_next(&soundCard) > -1)
	{
		if (soundCard < 0)
		{
			break;
		}

		char cardId[32];
		sprintf(cardId, "hw:%d", soundCard);

		if ((error = snd_ctl_open(&deviceHandle, cardId, SND_CTL_NONBLOCK)) < 0)
		{
			Error(_log, "Erorr opening device: (%i): %s", soundCard, snd_strerror(error));
			continue;
		}

		if ((error = snd_ctl_card_info(deviceHandle, cardInfo)) < 0)
		{
			Error(_log, "Erorr getting hardware info: (%i): %s", soundCard, snd_strerror(error));
			snd_ctl_close(deviceHandle);
			continue;
		}

		cardInput = -1;

		while (true)
		{
			if (snd_ctl_pcm_next_device(deviceHandle, &cardInput) < 0)
				Error(_log, "Error selecting device input");

			if (cardInput < 0)
				break;

			snd_pcm_info_set_device(deviceInfo, static_cast<uint>(cardInput));
			snd_pcm_info_set_subdevice(deviceInfo, 0);
			snd_pcm_info_set_stream(deviceInfo, SND_PCM_STREAM_CAPTURE);

			if ((error = snd_ctl_pcm_info(deviceHandle, deviceInfo)) < 0)
			{
				if (error != -ENOENT)
					Error(_log, "Digital Audio Info: (%i): %s", soundCard, snd_strerror(error));

				continue;
			}

			AudioGrabber::DeviceProperties device;

			device.id = QString("hw:%1,%2").arg(snd_pcm_info_get_card(deviceInfo)).arg(snd_pcm_info_get_device(deviceInfo));
			device.name = QString("%1: %2").arg(snd_ctl_card_info_get_name(cardInfo),snd_pcm_info_get_name(deviceInfo));

			Debug(_log, "Found sound card (%s): %s", QSTRING_CSTR(device.id), QSTRING_CSTR(device.name));

			_deviceProperties.insert(device.id, device);
		}

		snd_ctl_close(deviceHandle);
	}
}

bool AudioGrabberLinux::configureCaptureInterface()
{
	int error = -1;
	QString name = (_device.isEmpty() || _device == "auto") ? "default" : (_device);

	if ((error = snd_pcm_open(&_captureDevice, QSTRING_CSTR(name) , SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0)
	{
		Error(_log, "Failed to open audio device: %s, - %s", QSTRING_CSTR(_device), snd_strerror(error));
		return false;
	}

	if ((error = snd_pcm_hw_params_malloc(&_captureDeviceConfig)) < 0)
	{
		Error(_log, "Failed to create hardware parameters: %s", snd_strerror(error));
		snd_pcm_close(_captureDevice);
		return false;
	}

	if ((error = snd_pcm_hw_params_any(_captureDevice, _captureDeviceConfig)) < 0)
	{
		Error(_log, "Failed to initialize hardware parameters: %s", snd_strerror(error));
		snd_pcm_hw_params_free(_captureDeviceConfig);
		snd_pcm_close(_captureDevice);
		return false;
	}
	
	if ((error = snd_pcm_hw_params_set_access(_captureDevice, _captureDeviceConfig, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		Error(_log, "Failed to configure interleaved mode: %s", snd_strerror(error));
		snd_pcm_hw_params_free(_captureDeviceConfig);
		snd_pcm_close(_captureDevice);
		return false;
	}
	
	if ((error = snd_pcm_hw_params_set_format(_captureDevice, _captureDeviceConfig, SND_PCM_FORMAT_S16_LE)) < 0)
	{
		Error(_log, "Failed to configure capture format: %s", snd_strerror(error));
		snd_pcm_hw_params_free(_captureDeviceConfig);
		snd_pcm_close(_captureDevice);
		return false;
	}

	if ((error = snd_pcm_hw_params_set_rate_near(_captureDevice, _captureDeviceConfig, &_sampleRate, nullptr)) < 0)
	{
		Error(_log, "Failed to configure sample rate: %s", snd_strerror(error));
		snd_pcm_hw_params_free(_captureDeviceConfig);
		snd_pcm_close(_captureDevice);
		return false;
	}

	if ((error = snd_pcm_hw_params(_captureDevice, _captureDeviceConfig)) < 0)
	{
		Error(_log, "Failed to configure hardware parameters: %s", snd_strerror(error));
		snd_pcm_hw_params_free(_captureDeviceConfig);
		snd_pcm_close(_captureDevice);
		return false;
	}

	snd_pcm_hw_params_free(_captureDeviceConfig);

	if ((error = snd_pcm_prepare(_captureDevice)) < 0)
	{
		Error(_log, "Failed to prepare audio interface: %s", snd_strerror(error));
		snd_pcm_close(_captureDevice);
		return false;
	}

	if ((error = snd_pcm_start(_captureDevice)) < 0)
	{
		Error(_log, "Failed to start audio interface: %s", snd_strerror(error));
		snd_pcm_close(_captureDevice);
		return false;
	}
	
	return true;
}

bool AudioGrabberLinux::start()
{
	if (!_isEnabled)
		return false;

	if (_isRunning.load(std::memory_order_acquire))
		return true;

	Debug(_log, "Start Audio With %s", QSTRING_CSTR(getDeviceName(_device)));

	if (!configureCaptureInterface())
		return false;

	_isRunning.store(true, std::memory_order_release);

	pthread_attr_t threadAttributes;
	int threadPriority = 1;

	sched_param schedulerParameter;
	schedulerParameter.sched_priority = threadPriority;

	if (pthread_attr_init(&threadAttributes) != 0)
	{
		Debug(_log, "Failed to create thread attributes");
		stop();
		return false;
	}

	if (pthread_create(&_audioThread, &threadAttributes, static_cast<THREADFUNCPTR>(&AudioThreadRunner), static_cast<void*>(this)) != 0)
	{
		Debug(_log, "Failed to create audio capture thread");
		stop();
		return false;
	}

	AudioGrabber::start();

	return true;
}

void AudioGrabberLinux::stop()
{
	if (!_isRunning.load(std::memory_order_acquire))
		return;

	Debug(_log, "Stopping Audio Interface");

	_isRunning.store(false, std::memory_order_release);

	if (_audioThread != 0) {
		pthread_join(_audioThread, NULL);
	}

	snd_pcm_close(_captureDevice);

	AudioGrabber::stop();
}

void AudioGrabberLinux::processAudioBuffer(snd_pcm_sframes_t frames)
{
	if (!_isRunning.load(std::memory_order_acquire))
		return;

	ssize_t bytes = snd_pcm_frames_to_bytes(_captureDevice, frames);

	int16_t * buffer = static_cast<int16_t*>(calloc(static_cast<size_t>(bytes / 2), sizeof(int16_t)));
	
	if (frames == 0)
	{
		buffer[0] = 0;
		processAudioFrame(buffer, 1);
	}
	else
	{
		snd_pcm_sframes_t framesRead = snd_pcm_readi(_captureDevice, buffer,  static_cast<snd_pcm_uframes_t>(frames));

		if (framesRead < frames)
		{
			Error(_log, "Error reading audio. Got %d frames instead of %d", framesRead, frames);
		}
		else
		{
			processAudioFrame(buffer, static_cast<int>(snd_pcm_frames_to_bytes(_captureDevice, framesRead)) / 2);
		}
	}

	free(buffer);
}

QJsonArray AudioGrabberLinux::discover(const QJsonObject& /*params*/)
{
	refreshDevices();

	QJsonArray devices;

	for (auto deviceIterator = _deviceProperties.begin(); deviceIterator != _deviceProperties.end(); ++deviceIterator)
	{
		// Device
		QJsonObject device;
		QJsonArray deviceInputs;

		device["device"] = deviceIterator.key();
		device["device_name"] = deviceIterator.value().name;
		device["type"] = "audio";

		devices.append(device);
	}

	return devices;
}

QString AudioGrabberLinux::getDeviceName(const QString& devicePath) const
{
	if (devicePath.isEmpty() || devicePath == "auto")
	{
		return "Default Audio Device";
	}

	return _deviceProperties.value(devicePath).name;
}

static void * AudioThreadRunner(void* params)
{
	AudioGrabberLinux* This = static_cast<AudioGrabberLinux*>(params);

	Debug(This->getLog(), "Audio Thread Started");

	snd_pcm_sframes_t framesAvailable = 0;
			
	while (This->_isRunning.load(std::memory_order_acquire))
	{
		snd_pcm_wait(This->_captureDevice, 1000);

		if ((framesAvailable = snd_pcm_avail(This->_captureDevice)) > 0)
			This->processAudioBuffer(framesAvailable);

		sched_yield();
	}

	Debug(This->getLog(), "Audio Thread Shutting Down");
	return nullptr;
}
