#ifndef AUDIOGRABBERLINUX_H
#define AUDIOGRABBERLINUX_H

#include <pthread.h>
#include <sched.h>
#include <alsa/asoundlib.h>

// Hyperion-utils includes
#include <grabber/AudioGrabber.h>

class AudioGrabberLinux : public AudioGrabber
{

	// FIXME: Update and add descriptions for functions and class
	public:

		AudioGrabberLinux();

		void processAudioBuffer(snd_pcm_sframes_t frames);

		std::atomic<bool> _isRunning;
		snd_pcm_t * _captureDevice;

	public slots:

		bool start() override;
		void stop() override;
		QJsonArray discover(const QJsonObject& params) override;

	private:

		void refreshDevices();
		bool configureCaptureInterface();

		QString getDeviceName(const QString& devicePath) const;

		unsigned int _sampleRate;
		pthread_t _audioThread;

		snd_pcm_hw_params_t * _captureDeviceConfig;
};

static void* AudioThreadRunner(void* params);

#endif // AUDIOGRABBERLINUX_H
