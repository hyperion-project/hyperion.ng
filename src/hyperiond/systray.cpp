
#include <list>

#include <QPixmap>
#include <QWindow>
#include <QGuiApplication>
#include <QWidget>
#include <QColor>
#include <QDesktopServices>

#include <utils/ColorRgb.h>
#include <effectengine/EffectDefinition.h>

#include "hyperiond.h"
#include "systray.h"

SysTray::SysTray(HyperionDaemon *hyperiond, quint16 webPort)
	: QWidget()
	, _colorDlg(this)
	, _hyperiond(hyperiond)
	, _webPort(webPort)
{
	Q_INIT_RESOURCE(resource);
	_hyperion = Hyperion::getInstance();
	createTrayIcon();

	connect(_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
 
 	connect(&_colorDlg, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(setColor(const QColor &)));
	QIcon icon(":/hyperion-icon.png");
	_trayIcon->setIcon(icon);
	_trayIcon->show();
	setWindowIcon(icon);
	_colorDlg.setOptions(QColorDialog::NoButtons);
}

SysTray::~SysTray()
{
}

void SysTray::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
		case QSystemTrayIcon::Trigger:
			break;
		case QSystemTrayIcon::DoubleClick:
			showColorDialog();
			break;
		case QSystemTrayIcon::MiddleClick:
			break;
		default: ;
	}
}

void SysTray::createTrayIcon()
{
	quitAction = new QAction(tr("&Quit"), this);
	QIcon quitIcon = QIcon::fromTheme("application-exit");
	quitAction->setIcon(quitIcon);
	connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

	colorAction = new QAction(tr("&Color"), this);
	QIcon colorIcon = QIcon::fromTheme("applications-graphics");
	colorAction->setIcon(colorIcon);
	connect(colorAction, SIGNAL(triggered()), this, SLOT(showColorDialog()));

	settingsAction = new QAction(tr("&Settings"), this);
	QIcon settingsIcon = QIcon::fromTheme("preferences-system");
	settingsAction->setIcon(settingsIcon);
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(settings()));

	clearAction = new QAction(tr("&Clear"), this);
	QIcon clearIcon = QIcon::fromTheme("edit-delete");
	clearAction->setIcon(clearIcon);
	connect(clearAction, SIGNAL(triggered()), this, SLOT(clearEfxColor()));

	const std::list<EffectDefinition> efxs = _hyperion->getEffects();
	_trayIconMenu = new QMenu(this);
	_trayIconEfxMenu = new QMenu(_trayIconMenu);
	_trayIconEfxMenu->setTitle(tr("Effects"));
	QIcon efxIcon = QIcon::fromTheme("media-playback-start");
	_trayIconEfxMenu->setIcon(efxIcon);
	for (auto efx : efxs)
	{
		QAction *efxAction = new QAction(efx.name, this);
		connect(efxAction, SIGNAL(triggered()), this, SLOT(setEffect()));
		_trayIconEfxMenu->addAction(efxAction);
	}
	
	_trayIconMenu->addAction(settingsAction);
	_trayIconMenu->addSeparator();
	_trayIconMenu->addAction(colorAction);
	_trayIconMenu->addMenu(_trayIconEfxMenu);
	_trayIconMenu->addAction(clearAction);
	_trayIconMenu->addSeparator();
	_trayIconMenu->addAction(quitAction);

	_trayIcon = new QSystemTrayIcon(this);
	_trayIcon->setContextMenu(_trayIconMenu);
}

void SysTray::setColor(const QColor & color)
{
	ColorRgb rgbColor;
	rgbColor.red   = color.red();
	rgbColor.green = color.green();
 	rgbColor.blue  =color.blue();
 
 	_hyperion->setColor(1 ,rgbColor, 0);
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

void SysTray::settings()
{
	QDesktopServices::openUrl(QUrl("http://localhost:"+QString::number(_webPort)+"/", QUrl::TolerantMode));
}

void SysTray::setEffect()
{
	QString efxName = qobject_cast<QAction*>(sender())->text();
	_hyperion->setEffect(efxName, 1);
}

void SysTray::clearEfxColor()
{
	_hyperion->clear(1);
}
