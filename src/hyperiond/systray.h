#ifndef SYSTRAY_H
#define SYSTRAY_H

#ifdef Status
	#undef Status
#endif

#include <QSystemTrayIcon>
#include <QMenu>
#include <QWidget>
#include <QColorDialog>
#include <QCloseEvent>
#include <QSharedPointer>

#include <hyperion/HyperionIManager.h>
#include <QWeakPointer>
#include <events/EventHandler.h>

class HyperionDaemon;

class SysTray : public QSystemTrayIcon
{
	Q_OBJECT

public:
	explicit SysTray(HyperionDaemon* hyperiond);

private slots:

	void onIconActivated(QSystemTrayIcon::ActivationReason reason) const;

	void handleInstanceStateChange(InstanceState state, quint8 instance, const QString& name);

	void onWebserverPortChanged(quint16 port) { _webPort = port; }

#if defined(ENABLE_EFFECTENGINE)
	void onEffectListChanged();
#endif

private:
signals:
	void signalEvent(Event event);

private:
	void settings() const;
	void setAutorunState();

	void showColorDialog(quint8 instance);

	void setColor(quint8 instance, const QColor &color) const;
	void clearSource(quint8 instance) const;

#if defined(ENABLE_EFFECTENGINE)
	void setEffect(quint8 instance, const QString& effectName) const;
#endif

	void handleInstanceStarted(quint8 instance);
	void handleInstanceStopped(quint8 instance);

	QAction *createAction(const QString &text, const QString &iconPath, const std::function<void()> &method);

	// Helper Methods
	void setupConnections();
	void createBaseTrayMenu();


#ifdef _WIN32
	///
	/// @brief Checks whether Hyperion should start at Windows system start.
	/// @return True on success, otherwise false
	///
	bool getCurrentAutorunState();
#endif

	// Members
	HyperionDaemon* _hyperiond;
	QWeakPointer<HyperionIManager> _instanceManagerWeak;
	int _webPort = 8090;

	// UI Elements
	QMenu* _trayMenu;
	QMap<quint8, QMenu *> _instanceMenus; // Maps instance numbers to their menus

	// Actions
	QAction* _settingsAction;
	QAction* _suspendAction;
	QAction* _resumeAction;
	QAction* _restartAction;
	QAction* _quitAction;

#ifdef _WIN32
	QAction* _autorunAction;
#endif

	QColorDialog _colorDlg;
};

#endif // SYSTRAY_H
