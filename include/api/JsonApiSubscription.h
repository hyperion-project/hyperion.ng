#ifndef JSONAPISUBSCRIPTION_H
#define JSONAPISUBSCRIPTION_H

#include <HyperionConfig.h> // Required to determine the cmake options

#include <QMap>
#include <QString>


class Subscription {
public:
	enum Type {
		Unknown,
		AdjustmentUpdate,
		ComponentsUpdate,
#if defined(ENABLE_EFFECTENGINE)
		EffectsUpdate,
#endif
		ImageToLedMappingUpdate,
		ImageUpdate,
		InstanceUpdate,
		LedColorsUpdate,
		LedsUpdate,
		LogMsgUpdate,
		PrioritiesUpdate,
		SettingsUpdate,
		TokenUpdate,
		VideomodeUpdate
	};

	static QString toString(Type type) {
		switch (type) {
		case AdjustmentUpdate: return "adjustment-update";
		case ComponentsUpdate: return "components-update";
#if defined(ENABLE_EFFECTENGINE)
		case EffectsUpdate: return "effects-update";
#endif
		case ImageToLedMappingUpdate: return "imageToLedMapping-update";
		case ImageUpdate: return "image-update";
		case InstanceUpdate: return "instance-update";
		case LedColorsUpdate: return "led-colors-update";
		case LedsUpdate: return "leds-update";
		case LogMsgUpdate: return "logmsg-update";
		case PrioritiesUpdate: return "priorities-update";
		case SettingsUpdate: return "settings-update";
		case TokenUpdate: return "token-update";
		case VideomodeUpdate: return "videomode-update";
		default: return "unknown";
		}
	}
};

class JsonApiSubscription {
public:

	JsonApiSubscription()
		: cmd(Subscription::Unknown),
		  isAll(false)
	{}

	JsonApiSubscription(Subscription::Type cmd, bool isAll)
		: cmd(cmd),
		  isAll(isAll)
	{}

	Subscription::Type getSubscription() const { return cmd; }
	bool isPartOfAll() const { return isAll; }

	QString toString() const {
		return Subscription::toString(cmd);
	}

	Subscription::Type cmd;
	bool isAll;
};

typedef QMap<QString, JsonApiSubscription> SubscriptionLookupMap;

class ApiSubscriptionRegister {
public:

	static const SubscriptionLookupMap& getSubscriptionLookup() {
		static const SubscriptionLookupMap subscriptionLookup {
			{ {"adjustment-update"}, { Subscription::AdjustmentUpdate, true} },
			{ {"components-update"}, { Subscription::ComponentsUpdate, true} },
#if defined(ENABLE_EFFECTENGINE)
			{ {"effects-update"}, { Subscription::EffectsUpdate, true} },
#endif
			{ {"imageToLedMapping-update"}, { Subscription::ImageToLedMappingUpdate, true} },
			{ {"image-update"}, { Subscription::ImageUpdate, false} },
			{ {"instance-update"}, { Subscription::InstanceUpdate, true} },
			{ {"led-colors-update"}, { Subscription::LedColorsUpdate, true} },
			{ {"leds-update"}, { Subscription::LedsUpdate, false} },
			{ {"logmsg-update"}, { Subscription::LogMsgUpdate, false} },
			{ {"priorities-update"}, { Subscription::PrioritiesUpdate, true} },
			{ {"settings-update"}, { Subscription::SettingsUpdate, true} },
			{ {"token-update"}, { Subscription::TokenUpdate, true} },
			{ {"videomode-update"}, { Subscription::VideomodeUpdate, true} }
		};
		return subscriptionLookup;
	}

	static JsonApiSubscription getSubscriptionInfo(const QString& subscription) {
		return getSubscriptionLookup().value({subscription});
	}
};

#endif // JSONAPISUBSCRIPTION_H
