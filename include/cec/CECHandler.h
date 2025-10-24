#pragma once

#include <QObject>
#include <QVector>
#include <QMap>

#include <iostream>

#include <libcec/cec.h>

#include <utils/settings.h>
#include <events/EventEnum.h>

using CECCallbacks         = CEC::ICECCallbacks;
using CECAdapter           = CEC::ICECAdapter;
using CECAdapterDescriptor = CEC::cec_adapter_descriptor;
using CECLogMessage        = CEC::cec_log_message;
using CECKeyPress          = CEC::cec_keypress;
using CECCommand           = CEC::cec_command;
using CECLogicalAddress    = CEC::cec_logical_address;
using CECLogicalAddresses  = CEC::cec_logical_addresses;
using CECMenuState         = CEC::cec_menu_state;
using CECPowerStatus       = CEC::cec_power_status;
using CECVendorId          = CEC::cec_vendor_id;
using CECParameter         = CEC::libcec_parameter;
using CECConfig            = CEC::libcec_configuration;
using CECAlert             = CEC::libcec_alert;

class Logger;

class CECHandler : public QObject
{
	Q_OBJECT
public:
	CECHandler(const QJsonDocument& config, QObject * parent = nullptr);

	QString scan() const;

public slots:

	bool start();
	void stop();

	virtual void handleSettingsUpdate(settings::type type, const QJsonDocument& config);

private:

signals:
	void signalEvent(Event event);

private:

	bool enable();
	void disable();

	/* CEC Callbacks */
	static void onCecLogMessage           (void * context, const CECLogMessage * message);
	static void onCecKeyPress             (void * context, const CECKeyPress * key);
	static void onCecAlert                (void * context, const CECAlert alert, const CECParameter data);
	static void onCecConfigurationChanged (void * context, const CECConfig * configuration);
	static void onCecCommandReceived      (void * context, const CECCommand * command);
	static void onCecSourceActivated      (void * context, const CECLogicalAddress address, const uint8_t activated);
	static int  onCecMenuStateChanged     (void * context, const CECMenuState state);

	/* CEC Adapter helpers */
	QVector<CECAdapterDescriptor> getAdapters() const;
	bool openAdapter(const CECAdapterDescriptor & descriptor);
	void printAdapter(const CECAdapterDescriptor & descriptor) const;

	// CEC Event Strings per https://github.com/Pulse-Eight/libcec/blob/master/src/libcec/CECTypeUtils.h
	void triggerAction(const QString& cecEvent);

	QJsonDocument _config;

	/* CEC Helpers */
	CECCallbacks getCallbacks() const;
	CECConfig getConfig() const;

	CECAdapter * _cecAdapter   {};
	CECCallbacks _cecCallbacks {};
	CECConfig    _cecConfig    {};

	bool _isInitialised;
	bool _isOpen;
	bool _isEnabled;

	int _buttonReleaseDelayMs;
	int _buttonRepeatRateMs;
	int _doubleTapTimeoutMs;

	QMap<QString,Event> _cecEventActionMap;

	QSharedPointer<Logger> _logger;
};
