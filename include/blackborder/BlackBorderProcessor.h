#pragma once

// QT includes
#include <QJsonObject>

// util
#include <utils/Logger.h>
#include <utils/settings.h>
#include <utils/Components.h>

// Local Hyperion includes
#include "BlackBorderDetector.h"

class Hyperion;

namespace hyperion
{
	///
	/// The BlackBorder processor is a wrapper around the black-border detector for keeping track of
	/// detected borders and count of the type and size of detected borders.
	///
	class BlackBorderProcessor : public QObject
	{
		Q_OBJECT
	public:
		BlackBorderProcessor(Hyperion* hyperion, QObject* parent);
		~BlackBorderProcessor() override;
		///
		/// Return the current (detected) border
		/// @return The current border
		///
		BlackBorder getCurrentBorder() const;

		///
		/// Return activation state of black border detector
		/// @return The current border
		///
		bool enabled() const;

		///
		/// Set activation state of black border detector
		/// @param enable current state
		///
		void setEnabled(bool enable);

		///
		/// Sets the _hardDisabled state, if True prevents the enable from COMP_BLACKBORDER state emit (mimics wrong state to external!)
		/// It's not possible to enable black-border detection from this method, if the user requested a disable!
		/// @param disable  The new state
		///
		void setHardDisable(bool disable);

		///
		/// Processes the image. This performs detection of black-border on the given image and
		/// updates the current border accordingly. If the current border is updated the method call
		/// will return true else false
		///
		/// @param image The image to process
		///
		/// @return True if a different border was detected than the current else false
		///
		template <typename Pixel_T>
		bool process(const Image<Pixel_T> & image)
		{
			// get the border for the single image
			BlackBorder imageBorder;
			imageBorder.horizontalSize = 0;
			imageBorder.verticalSize = 0;

			if (!enabled())
			{
				imageBorder.unknown=true;
				_currentBorder = imageBorder;
				return true;
			}

			if (_detectionMode == "default") {
				imageBorder = _detector->process(image);
			} else if (_detectionMode == "classic") {
				imageBorder = _detector->process_classic(image);
			} else if (_detectionMode == "osd") {
				imageBorder = _detector->process_osd(image);
			} else if (_detectionMode == "letterbox") {
				imageBorder = _detector->process_letterbox(image);
			}
			// add blur to the border
			if (imageBorder.horizontalSize > 0)
			{
				imageBorder.horizontalSize += _blurRemoveCnt;
			}
			if (imageBorder.verticalSize > 0)
			{
				imageBorder.verticalSize += _blurRemoveCnt;
			}

			const bool borderUpdated = updateBorder(imageBorder);
			return borderUpdated;
		}
	private slots:
		///
		/// @brief Handle settings update from Hyperion Settingsmanager emit or this constructor
		/// @param type   settingType from enum
		/// @param config configuration object
		///
		void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

		///
		/// @brief Handle component state changes, it's not possible for BB to be enabled, when a hardDisable is active
		///
		void handleCompStateChangeRequest(hyperion::Components component, bool enable);

	private:
		/// Hyperion instance
		Hyperion* _hyperion;

		///
		/// Updates the current border based on the newly detected border. Returns true if the
		/// current border has changed.
		///
		/// @param newDetectedBorder  The newly detected border
		/// @return True if the current border changed else false
		///
		bool updateBorder(const BlackBorder & newDetectedBorder);

		/// flag for black-border detector usage
		bool _enabled;

		/// The number of unknown-borders detected before it becomes the current border
		unsigned _unknownSwitchCnt;

		/// The number of horizontal/vertical borders detected before it becomes the current border
		unsigned _borderSwitchCnt;

		// The number of frames that are "ignored" before a new border gets set as _previousDetectedBorder
		unsigned _maxInconsistentCnt;

		/// The number of pixels to increase a detected border for removing blurry pixels
		unsigned _blurRemoveCnt;

		/// The border detection mode
		QString _detectionMode;

		/// The black-border detector
		BlackBorderDetector* _detector;

		/// The current detected border
		BlackBorder _currentBorder;

		/// The border detected in the previous frame
		BlackBorder _previousDetectedBorder;

		/// The number of frame the previous detected border matched the incoming border
		unsigned _consistentCnt;
		/// The number of frame the previous detected border NOT matched the incoming border
		unsigned _inconsistentCnt;
		/// old threshold
		double _oldThreshold;
		/// True when disabled in specific situations, this prevents to enable BB when the visible priority requested a disable
		bool _hardDisabled;
		/// Reflect the last component state request from user (comp change)
		bool _userEnabled;

	};
} // end namespace hyperion
