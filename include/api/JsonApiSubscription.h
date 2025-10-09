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
		EventUpdate,
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
		case EventUpdate: return "event-update";
		case ImageToLedMappingUpdate: return "imageToLedMapping-update";
		case ImageUpdate: return "ledcolors-imagestream-update";
		case InstanceUpdate: return "instance-update";
		case LedColorsUpdate: return "ledcolors-ledstream-update";
		case LedsUpdate: return "leds-update";
		case LogMsgUpdate: return "logmsg-update";
		case PrioritiesUpdate: return "priorities-update";
		case SettingsUpdate: return "settings-update";
		case TokenUpdate: return "token-update";
		case VideomodeUpdate: return "videomode-update";
		default: return "unknown";
		}
	}

	static bool isInstanceSpecific(Type type) {
		switch (type) {
		case AdjustmentUpdate:
		case ComponentsUpdate:
#if defined(ENABLE_EFFECTENGINE)
		case EffectsUpdate:
#endif
		case ImageToLedMappingUpdate:
		case ImageUpdate:
		case LedColorsUpdate:
		case LedsUpdate:
		case PrioritiesUpdate:
		case SettingsUpdate:
		return true;
		case EventUpdate:
		case InstanceUpdate:
		case LogMsgUpdate:
		case TokenUpdate:
		case VideomodeUpdate:
		default:
		return false;
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
			{ {"event-update"}, { Subscription::EventUpdate, true} },
			{ {"imageToLedMapping-update"}, { Subscription::ImageToLedMappingUpdate, true} },
			{ {"ledcolors-imagestream-update"}, { Subscription::ImageUpdate, false} },
			{ {"ledcolors-ledstream-update"}, { Subscription::LedColorsUpdate, false} },
			{ {"instance-update"}, { Subscription::InstanceUpdate, true} },
			{ {"leds-update"}, { Subscription::LedsUpdate, true} },
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
