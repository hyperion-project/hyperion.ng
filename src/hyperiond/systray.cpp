#include "systray.h"

#ifndef _WIN32
#include <unistd.h>
#endif

#include "hyperiond.h"
#include <webserver/WebServer.h>
#include <events/EventHandler.h>

#include <HyperionConfig.h> // Required to determine the cmake options
#include <hyperion/Hyperion.h>
#if defined(ENABLE_EFFECTENGINE)
#include <effectengine/EffectDefinition.h>
#include <effectengine/EffectFileHandler.h>
#endif

#include <QDesktopServices>
#include <QSettings>
 
SysTray::SysTray(HyperionDaemon* hyperiond)
	: QSystemTrayIcon(QIcon(":/hyperion-32px.png"), hyperiond)
	, _hyperiond(hyperiond)
	, _instanceManagerWeak(HyperionIManager::getInstanceWeak())
	, _webPort(8090)
	, _colorDlg(nullptr)
{
	Q_INIT_RESOURCE(resources);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

	setupConnections();
	createBaseTrayMenu();

	// Delay showing the tray icon
	QTimer::singleShot(0, this, [this]() {
		this->show();
	});
	connect(this, &QSystemTrayIcon::activated, this, &SysTray::onIconActivated);
}

void SysTray::onIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
#ifdef _WIN32
	case QSystemTrayIcon::Context:
		getCurrentAutorunState();
	break;
#endif
	case QSystemTrayIcon::DoubleClick:
		settings();
	break;
	case QSystemTrayIcon::Trigger:
	case QSystemTrayIcon::MiddleClick:
	break;
	default: ;
	}
}

void SysTray::createBaseTrayMenu()
{
	_trayMenu = new QMenu();

	// Create actions
	_settingsAction = createAction(tr("&Settings"), ":/settings.svg", [this]() {
		settings();
	});

	_suspendAction = createAction(tr("&Suspend"), ":/suspend.svg", [this]() {
		emit signalEvent(Event::Suspend);
	});

	_resumeAction = createAction(tr("&Resume"), ":/resume.svg", [this]() {
		emit signalEvent(Event::Resume);
	});

	_restartAction = createAction(tr("&Restart"), ":/restart.svg", [this]() {
		emit signalEvent(Event::Restart);
	});

	_quitAction = createAction(tr("&Quit"), ":/quit.svg", []() {
		QApplication::quit();
	});

#ifdef _WIN32
	_autorunAction = createAction(tr("&Disable autostart"), ":/autorun.svg", [this]() {
		setAutorunState();
	});
#endif

	// Add static actions to the tray menu
	_trayMenu->addAction(_settingsAction);
#ifdef _WIN32
	_trayMenu->addAction(_autorunAction);
#endif

	// Add remaining static actions
	_trayMenu->addSeparator();
	_trayMenu->addAction(_suspendAction);
	_trayMenu->addAction(_resumeAction);
	_trayMenu->addAction(_restartAction);
	_trayMenu->addAction(_quitAction);

	setContextMenu(_trayMenu);
}

void SysTray::setupConnections()
{
	WebServer const * webserver = _hyperiond->getWebServerInstance();
	connect(webserver, &WebServer::portChanged, this, &SysTray::onWebserverPortChanged);
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		connect(mgr.get(), &HyperionIManager::instanceStateChanged, this, &SysTray::handleInstanceStateChange);
	}
	connect(this, &SysTray::signalEvent, EventHandler::getInstance().get(), &EventHandler::handleEvent);
}

QAction *SysTray::createAction(const QString &text, const QString &iconPath, const std::function<void()> &method)
{
	auto* action = new QAction(text, this);
	action->setIcon(QIcon(iconPath));
	connect(action, &QAction::triggered, this, method);
	return action;
}

void SysTray::setColor(quint8 instance, const QColor &color) const
{
	QSharedPointer<Hyperion> hyperion;
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		hyperion = mgr->getHyperionInstance(instance);
	}
	if (!hyperion.isNull())
	{
		QVector<ColorRgb> rgbColor{ ColorRgb(color.rgb()) };

		emit hyperion->setColor(PriorityMuxer::FG_PRIORITY,rgbColor, PriorityMuxer::ENDLESS);
	}
}

#if defined(ENABLE_EFFECTENGINE)
void SysTray::setEffect(quint8 instance, const QString& effectName) const
{
	QSharedPointer<Hyperion> hyperion;
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		hyperion = mgr->getHyperionInstance(instance);
	}
	if (!hyperion.isNull())
	{
		emit hyperion->setEffect(effectName, PriorityMuxer::FG_PRIORITY, PriorityMuxer::ENDLESS);
	}
}
#endif

void SysTray::showColorDialog(quint8 instance)
{
	if (_colorDlg.isVisible())
	{
		_colorDlg.hide();
	}
	else
	{
		QColor selectedColor = QColorDialog::getColor (Qt::white, nullptr, tr("Select Color"));
		if (selectedColor.isValid())
		{
			setColor(instance, selectedColor);
		}
	}
}

void SysTray::clearSource(quint8 instance) const
{
	QSharedPointer<Hyperion> hyperion;
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		hyperion = mgr->getHyperionInstance(instance);
	}
	if (!hyperion.isNull())
	{
		emit hyperion->clear(PriorityMuxer::FG_PRIORITY);
	}
}

void SysTray::handleInstanceStarted(quint8 instance)
{
	// Check if the instance already exists
	if (_instanceMenus.contains(instance))
	{
		return;
	}

	// Create a new menu for this instance
	QString instanceName;
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		instanceName = mgr->getInstanceName(instance);
	}
	auto* instanceMenu = new QMenu(instanceName);

	// Create actions for the instance menu
	QAction *colorAction = createAction(tr("&Color"), ":/color.svg", [this, instance]() {
		showColorDialog(instance);
	});
	instanceMenu->addAction(colorAction);

#if defined(ENABLE_EFFECTENGINE)
	// Get the list of effects
	const QList<EffectDefinition> effectsDefinitions = EffectFileHandler::getInstance()->getEffects();

	if(!effectsDefinitions.empty())
	{
		// Effects submenu
		auto* effectsMenu = new QMenu(tr("Effects"), instanceMenu);
		effectsMenu->setObjectName("effectsMenu");
		instanceMenu->addMenu(effectsMenu);

		// Add effects when the menu is first created
		for (const auto& effect : effectsDefinitions)
		{
			QAction const * effectAction = effectsMenu->addAction(effect.name);
			connect(effectAction, &QAction::triggered, [this, instance, effectName = effect.name]() {
				setEffect(instance, effectName);
			});
		}
		if (auto eff = EffectFileHandler::getInstance())
		{
			connect(eff.get(), &EffectFileHandler::effectListChanged, this, &SysTray::onEffectListChanged);
		}
	}
#endif

	QAction *clearAction = createAction(tr("&Clear"), ":/clear.svg", [instance, this]() {
		clearSource(instance);
	});
	instanceMenu->addAction(clearAction);

	_trayMenu->insertMenu(_suspendAction, instanceMenu);

	// Store the menu for later reference
	_instanceMenus[instance] = instanceMenu;
}

void SysTray::handleInstanceStopped(quint8 instance)
{
	// Check if the instance exists
	if (!_instanceMenus.contains(instance))
		return;

	// Remove the menu for this instance
	QMenu *instanceMenu = _instanceMenus.take(instance);
	_trayMenu->removeAction(instanceMenu->menuAction());

	// Delete the menu to free memory
	delete instanceMenu;
}

void SysTray::handleInstanceStateChange(InstanceState state, quint8 instance, const QString& /*name*/)
{
	switch (state)
	{
	case InstanceState::H_STARTED:
		handleInstanceStarted(instance);
	break;
	case InstanceState::H_STOPPED:
		handleInstanceStopped(instance);
	break;
	default:
	break;
	}
}

#if defined(ENABLE_EFFECTENGINE)
void SysTray::onEffectListChanged()
{
	// Get the updated list of effects
	const QList<EffectDefinition> effectsDefinitions = EffectFileHandler::getInstance()->getEffects();

	for (auto it = _instanceMenus.begin(); it != _instanceMenus.end(); ++it)
	{
		QMenu const * instanceMenu = it.value(); // Access the value (QMenu*) from the map
		quint8 instanceNumber = it.key();

		QMenu* effectsMenu = instanceMenu->findChild<QMenu*>("effectsMenu");

		if (effectsMenu)
		{
			// Clear existing effects
			effectsMenu->clear();

			// Re-add the updated list of effects
			for (const auto& effect : effectsDefinitions)
			{
				QAction const * effectAction = effectsMenu->addAction(effect.name);
				connect(effectAction, &QAction::triggered, [this, instance = instanceNumber, effectName = effect.name]() {
					setEffect(instance, effectName);
				});
			}
		}
	}
}
#endif

void SysTray::settings() const
{
#ifndef _WIN32
	// Hide error messages when opening webbrowser

	int out_pipe[2];
	int saved_stdout;
	int saved_stderr;

	// saving stdout and stderr file descriptor
	saved_stdout = ::dup( STDOUT_FILENO );
	saved_stderr = ::dup( STDERR_FILENO );

	if(::pipe(out_pipe) == 0)
	{
		// redirecting stdout to pipe
		::dup2(out_pipe[1], STDOUT_FILENO);
		::close(out_pipe[1]);
		// redirecting stderr to stdout
		::dup2(STDOUT_FILENO, STDERR_FILENO);
	}
#endif

	QDesktopServices::openUrl(QUrl("http://localhost:"+QString::number(_webPort)+"/", QUrl::TolerantMode));

#ifndef _WIN32
	// restoring stdout
	::dup2(saved_stdout, STDOUT_FILENO);
	// restoring stderr
	::dup2(saved_stderr, STDERR_FILENO);
#endif
}

#ifdef _WIN32
bool SysTray::getCurrentAutorunState()
{
	const QSettings reg("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
	if (reg.value("Hyperion", 0).toString() == QApplication::applicationFilePath().replace('/', '\\'))
	{
		_autorunAction->setText(tr("&Disable autostart"));
		return true;
	}

	_autorunAction->setText(tr("&Enable autostart"));
	return false;
}

void SysTray::setAutorunState()
{
	bool currentState = getCurrentAutorunState();
	QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
	(currentState)
			? reg.remove("Hyperion")
			: reg.setValue("Hyperion", QApplication::applicationFilePath().replace('/', '\\'));
}
#endif
