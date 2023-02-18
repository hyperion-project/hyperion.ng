#ifndef AUDIOGRABBERLINUX_H
#define AUDIOGRABBERLINUX_H

#include <pthread.h>
#include <sched.h>
#include <alsa/asoundlib.h>

// Hyperion-utils includes
#include <grabber/AudioGrabber.h>

///
/// @brief The Linux Audio capture implementation
///
class AudioGrabberLinux : public AudioGrabber
{
	public:

		AudioGrabberLinux();
		~AudioGrabberLinux() override;

		/// 
		/// Process audio buffer
		///
		void processAudioBuffer(snd_pcm_sframes_t frames);

		/// 
		/// Is Running Flag
		///
		std::atomic<bool> _isRunning;

		/// 
		/// Current capture device
		///
		snd_pcm_t * _captureDevice;

	public slots:

		/// 
		/// Start audio capturing session
		///
		/// @returns true if successful
		bool start() override;

		/// 
		/// Stop audio capturing session
		///
		void stop() override;

		/// 
		/// Discovery audio devices
		///
		QJsonArray discover(const QJsonObject& params) override;

	private:
		/// 
		/// Refresh audio devices
		///
		void refreshDevices();

		/// 
		/// Configure current audio capture interface
		///
		bool configureCaptureInterface();

		/// 
		/// Get device name from path
		///
		QString getDeviceName(const QString& devicePath) const;

		/// 
		/// Current sample rate
		///
		unsigned int _sampleRate;

		/// 
		/// Audio capture thread
		///
		pthread_t _audioThread;

		/// 
		/// ALSA device configuration parameters
		///
		snd_pcm_hw_params_t * _captureDeviceConfig;
};

/// 
/// Audio processing thread function
///
static void* AudioThreadRunner(void* params);

#endif // AUDIOGRABBERLINUX_H
