#ifndef SYSTRAY_H
#define SYSTRAY_H

#ifdef Status
	#undef Status
#endif

#include <QSystemTrayIcon>
#include <QMenu>
#include <QWidget>
#include <QWidgetAction>
#include <QFrame>
#include <QColorDialog>
#include <QCloseEvent>
#include <QSharedPointer>
#include <QToolButton>

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

	void onIconActivated(QSystemTrayIcon::ActivationReason reason);

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

	void updateStartupSourceIndicator();
	void syncSuspendButton();
	QColor getInitialDialogColor(quint8 instance) const;

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

	// Bottom inline action row (Suspend/Resume toggle, Restart, Quit)
	QWidgetAction* _bottomActionsRow = nullptr;
	QToolButton* _suspendResumeBtn = nullptr;

	bool _darkTheme = false;

	// First instance inline buttons (horizontal row instead of submenu)
	quint8 _firstInstanceNumber = 0;
	QWidgetAction* _firstInstanceAction = nullptr;
	QMenu* _firstEffectsMenu = nullptr;
	QColor _lastColor = Qt::white;

#ifdef _WIN32
	QAction* _autorunAction;
#endif

	// Indicator strips
	QFrame* _colorIndicator = nullptr;
	QFrame* _effectsIndicator = nullptr;

	QColorDialog _colorDlg;
};

#endif // SYSTRAY_H
