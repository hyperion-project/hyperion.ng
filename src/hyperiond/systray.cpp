
#include <list>
#ifndef _WIN32
#include <unistd.h>
#endif

// QT includes
#include <QPixmap>
#include <QWindow>
#include <QGuiApplication>
#include <QWidget>
#include <QColor>
#include <QDesktopServices>
#include <QSettings>
#include <QtGlobal>

#include <utils/ColorRgb.h>
#include <utils/Process.h>
#if defined(ENABLE_EFFECTENGINE)
#include <effectengine/EffectDefinition.h>
#include <effectengine/Effect.h>
#endif
#include <webserver/WebServer.h>
#include <hyperion/PriorityMuxer.h>

#include "hyperiond.h"
#include "systray.h"
#include "SuspendHandler.h"

SysTray::SysTray(HyperionDaemon *hyperiond)
	: QWidget()
	, _colorDlg(this)
	, _hyperiond(hyperiond)
	, _hyperion(nullptr)
	, _instanceManager(HyperionIManager::getInstance())
	, _suspendHandler (hyperiond->getSuspendHandlerInstance())
	, _webPort(8090)
{
	Q_INIT_RESOURCE(resources);

	// webserver port
	WebServer* webserver = hyperiond->getWebServerInstance();
	connect(webserver, &WebServer::portChanged, this, &SysTray::webserverPortChanged);

	// instance changes
	connect(_instanceManager, &HyperionIManager::instanceStateChanged, this, &SysTray::handleInstanceStateChange);
}

SysTray::~SysTray()
{
}

void SysTray::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
#ifdef _WIN32
		case QSystemTrayIcon::Context:
			getCurrentAutorunState();
			break;
#endif
		case QSystemTrayIcon::Trigger:
			break;
		case QSystemTrayIcon::DoubleClick:
			settings();
			break;
		case QSystemTrayIcon::MiddleClick:
			break;
		default: ;
	}
}

void SysTray::createTrayIcon()
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

	quitAction = new QAction(tr("&Quit"), this);
	quitAction->setIcon(QPixmap(":/quit.svg"));
	connect(quitAction, &QAction::triggered, qApp, QApplication::quit);

	restartAction = new QAction(tr("&Restart"), this);
	restartAction->setIcon(QPixmap(":/restart.svg"));
	connect(restartAction, &QAction::triggered, this , [=](){ Process::restartHyperion(12); });

	suspendAction = new QAction(tr("&Suspend"), this);
	suspendAction->setIcon(QPixmap(":/suspend.svg"));
	connect(suspendAction, &QAction::triggered, _suspendHandler, QOverload<>::of(&SuspendHandler::suspend));

	resumeAction = new QAction(tr("&Resume"), this);
	resumeAction->setIcon(QPixmap(":/resume.svg"));
	connect(resumeAction, &QAction::triggered, _suspendHandler, &SuspendHandler::resume);

	colorAction = new QAction(tr("&Color"), this);
	colorAction->setIcon(QPixmap(":/color.svg"));
	connect(colorAction, &QAction::triggered, this, &SysTray::showColorDialog);

	settingsAction = new QAction(tr("&Settings"), this);
	settingsAction->setIcon(QPixmap(":/settings.svg"));
	connect(settingsAction, &QAction::triggered, this, &SysTray::settings);

	clearAction = new QAction(tr("&Clear"), this);
	clearAction->setIcon(QPixmap(":/clear.svg"));
	connect(clearAction, &QAction::triggered, this, &SysTray::clearEfxColor);

	_trayIconMenu = new QMenu(this);

#if defined(ENABLE_EFFECTENGINE)
	const std::list<EffectDefinition> efxs = _hyperion->getEffects();
	_trayIconEfxMenu = new QMenu(_trayIconMenu);
	_trayIconEfxMenu->setTitle(tr("Effects"));
	_trayIconEfxMenu->setIcon(QPixmap(":/effects.svg"));

	// custom effects
	for (const auto &efx : efxs)
	{
		if (efx.file.mid(0, 1)  != ":")
		{
			QAction *efxAction = new QAction(efx.name, this);
			connect(efxAction, &QAction::triggered, this, &SysTray::setEffect);
			_trayIconEfxMenu->addAction(efxAction);
		}
	}

	// add seperator if custom effects exists
	if (!_trayIconEfxMenu->isEmpty())
		_trayIconEfxMenu->addSeparator();

	// build in effects
	for (const auto &efx : efxs)
	{
		if (efx.file.mid(0, 1)  == ":")
		{
			QAction *efxAction = new QAction(efx.name, this);
			connect(efxAction, &QAction::triggered, this, &SysTray::setEffect);
			_trayIconEfxMenu->addAction(efxAction);
		}
	}
#endif

#ifdef _WIN32
	autorunAction = new QAction(tr("&Disable autostart"), this);
	autorunAction->setIcon(QPixmap(":/autorun.svg"));
	connect(autorunAction, &QAction::triggered, this, &SysTray::setAutorunState);

	_trayIconMenu->addAction(autorunAction);
	_trayIconMenu->addSeparator();
#endif

	_trayIconMenu->addAction(settingsAction);
	_trayIconMenu->addSeparator();
	_trayIconMenu->addAction(colorAction);
#if defined(ENABLE_EFFECTENGINE)
	_trayIconMenu->addMenu(_trayIconEfxMenu);
#endif
	_trayIconMenu->addAction(clearAction);
	_trayIconMenu->addSeparator();

	_trayIconSystemMenu = new QMenu(_trayIconMenu);
	_trayIconSystemMenu->setTitle(tr("Instances"));

	_trayIconSystemMenu->addAction(suspendAction);
	_trayIconSystemMenu->addAction(resumeAction);
	_trayIconSystemMenu->addAction(restartAction);
	_trayIconMenu->addMenu(_trayIconSystemMenu);

	_trayIconMenu->addSeparator();
	_trayIconMenu->addAction(quitAction);

	_trayIcon = new QSystemTrayIcon(this);
	_trayIcon->setContextMenu(_trayIconMenu);
}

#ifdef _WIN32
bool SysTray::getCurrentAutorunState()
{
	QSettings reg("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
	if (reg.value("Hyperion", 0).toString() == qApp->applicationFilePath().replace('/', '\\'))
	{
		autorunAction->setText(tr("&Disable autostart"));
		return true;
	}

	autorunAction->setText(tr("&Enable autostart"));
	return false;
}
#endif

void SysTray::setAutorunState()
{
#ifdef _WIN32
	bool currentState = getCurrentAutorunState();
	QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
	(currentState)
	? reg.remove("Hyperion")
	: reg.setValue("Hyperion", qApp->applicationFilePath().replace('/', '\\'));
#endif
}

void SysTray::setColor(const QColor & color)
{
	std::vector<ColorRgb> rgbColor{ ColorRgb{ static_cast<uint8_t>(color.red()), static_cast<uint8_t>(color.green()), static_cast<uint8_t>(color.blue()) } };

	_hyperion->setColor(PriorityMuxer::FG_PRIORITY,rgbColor, PriorityMuxer::ENDLESS);
}

void SysTray::showColorDialog()
{
	if(_colorDlg.isVisible())
	{
		_colorDlg.hide();
	}
	else
	{
		_colorDlg.show();
	}
}

void SysTray::closeEvent(QCloseEvent *event)
{
	event->ignore();
}

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

#if defined(ENABLE_EFFECTENGINE)
void SysTray::setEffect()
{
	QString efxName = qobject_cast<QAction*>(sender())->text();
	_hyperion->setEffect(efxName, PriorityMuxer::FG_PRIORITY, PriorityMuxer::ENDLESS);
}
#endif

void SysTray::clearEfxColor()
{
	_hyperion->clear(1);
}

void SysTray::handleInstanceStateChange(InstanceState state, quint8 instance, const QString& /*name*/)
{
	switch(state){
		case InstanceState::H_STARTED:
			if(instance == 0)
			{
				_hyperion = _instanceManager->getHyperionInstance(0);

				createTrayIcon();
				connect(_trayIcon, &QSystemTrayIcon::activated, this, &SysTray::iconActivated);
				connect(quitAction, &QAction::triggered, _trayIcon, &QSystemTrayIcon::hide, Qt::DirectConnection);
				connect(&_colorDlg, &QColorDialog::currentColorChanged, this, &SysTray::setColor);

				QIcon icon(":/hyperion-icon-32px.png");
				_trayIcon->setIcon(icon);
				_trayIcon->show();
				setWindowIcon(icon);
				_colorDlg.setOptions(QColorDialog::NoButtons);
			}

			break;
		default:
			break;
	}
}
