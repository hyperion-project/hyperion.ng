#ifndef AUDIOGRABBER_H
#define AUDIOGRABBER_H

#include <QObject>
#include <QColor>
#include <cmath>

// Hyperion-utils includes
#include <utils/ColorRgb.h>
#include <hyperion/Grabber.h>
#include <utils/Logger.h>

///
/// Base Audio Grabber Class
///
/// This class is extended by the windows audio grabber to provied DirectX9 access to the audio devices
/// This class is extended by the linux audio grabber to provide ALSA access to the audio devices
///
/// @brief The DirectX9 capture implementation
///
class AudioGrabber : public Grabber
{
	Q_OBJECT
	public:

	///
	/// Device properties
	///
	/// this structure holds the name, id, and inputs of the enumerated audio devices. 
	///
	struct DeviceProperties
	{
		QString	name = QString();
		QString	id = QString();
		QMultiMap<QString, int>	inputs = QMultiMap<QString, int>();
	};

		AudioGrabber();
		~AudioGrabber() override;

		/// 
		/// Start audio capturing session
		///
		/// @returns true if successful
		virtual bool start();

		///
		/// Stop audio capturing session
		///
		virtual void stop();

		/// 
		/// Restart the audio capturing session
		/// 
		void restart();

		Logger* getLog();

		/// 
		/// Set Device
		///
		/// configures the audio device used by the grabber
		/// 
		/// @param[in] device identifier of audio device
		void setDevice(const QString& device);

		/// 
		/// Set Configuration
		///
		/// sets the audio grabber's configuration parameters
		///
		/// @param[in] config object of configuration parameters
		void setConfiguration(const QJsonObject& config);

		/// 
		/// Reset Multiplier
		///
		/// resets the calcualted audio multiplier so that it is recalculated
		/// currently the multiplier is only reduced based on loudness.
		///
		/// TODO: also calculate a low signal and reset the multiplier
		/// 
		void resetMultiplier();

		/// 
		/// Discover
		///
		/// discovers audio devices in the system
		///
		/// @param[in] params discover parameters
		/// @return array of audio devices
		virtual QJsonArray discover(const QJsonObject& params);
		
	signals:
		void newFrame(const Image<ColorRgb>& image);

	protected:

		///
		/// Process Audio Frame
		///
		/// this functions takes in an audio buffer and emits a visual representation of the audio data
		///
		/// @param[in] buffer The audio buffer to process
		/// @param[in] length The length of audio data in the buffer
		void processAudioFrame(int16_t* buffer, int length);

		/// 
		/// Audio device id / properties map
		///
		/// properties include information such as name, inputs, and etc...
		/// 
		QMap<QString, AudioGrabber::DeviceProperties> _deviceProperties;

		/// 
		/// Current device
		/// 
		QString _device;

		/// 
		/// Hot Color
		///
		/// the color of the leds when the signal is high or hot 
		///
		QColor _hotColor;

		///
		/// Warn value
		///
		/// The maximum value of the warning color. above this threshold the signal is considered hot 
		///
		int _warnValue;

		/// 
		/// Warn color
		///
		/// the color of the leds when the signal is in between the safe and warn value threshold
		/// 
		QColor _warnColor;

		///
		/// Save value
		///
		/// The maximum value of the safe color. above this threshold the signal enteres the warn zone.
		/// below the signal is in the safe zone. 
		///
		int _safeValue;

		///
		/// Safe color
		///
		/// the color of the leds when the signal is below the safe threshold
		///
		QColor _safeColor;

		///
		/// Multiplier
		///
		/// this value is used to multiply the input signal value. Some inputs may have a very low signal
		/// and the multiplier is used to get the desired visualization.
		///
		/// When the multiplier is configured to 0, the multiplier is automatically configured based off of the average
		/// signal amplitude and tolernace. 
		///
		double _multiplier;

		///
		/// Tolerance
		///
		/// The tolerance is used to calculate what percentage of the top end part of the signal to ignore when
		/// calculating the multiplier. This enables the effect to reach the hot zone with an auto configured multiplier 
		///
		int _tolerance;

		///
		/// Dynamic Multiplier
		///
		/// This is the current value of the automatically configured multiplier. 
		///
		double _dynamicMultiplier;

		///
		/// Started
		///
		/// true if the capturing session has started. 
		///
		bool _started;

private:
	///
	/// @brief free the _screen pointer
	///
	void freeResources();
};

#endif // AUDIOGRABBER_H
