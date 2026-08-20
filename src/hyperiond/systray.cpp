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
#include <QWidgetAction>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QPainter>
#include <utils/settings.h>

namespace {
	QIcon recoloredIcon(const QString &svgPath, const QColor &color, int size = 18)
	{
		QIcon svgIcon(svgPath);
		QImage img = svgIcon.pixmap(QSize(size, size)).toImage().convertToFormat(QImage::Format_ARGB32);
		for (int y = 0; y < img.height(); y++)
		{
			QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y));
			for (int x = 0; x < img.width(); x++)
			{
				int a = qAlpha(line[x]);
				if (a > 0)
					line[x] = qRgba(color.red(), color.green(), color.blue(), a);
			}
		}
		return QIcon(QPixmap::fromImage(img));
	}

	}
 
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

	// Detect dark theme
	QPalette pal = QGuiApplication::palette();
	_darkTheme = pal.color(QPalette::Window).lightness() < 128;

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

	// Dark theme styling
	if (_darkTheme)
	{
		_trayMenu->setStyleSheet(
			"QMenu { background-color: #2b2b2b; color: #e0e0e0; border: 1px solid #555; padding: 6px 0; }"
			"QMenu::separator { height: 1px; background: #555; margin: 4px 8px; }"
		);
	}
	else
	{
		_trayMenu->setStyleSheet(
			"QMenu { padding: 6px 0; }"
		);
	}

	// Create actions
	_settingsAction = createAction(tr("&Settings"), ":/settings.svg", [this]() {
		settings();
	});

#ifdef _WIN32
	_autorunAction = createAction(tr("&Enable autostart"), ":/autorun.svg", [this]() {
		setAutorunState();
	});
#endif

	// Add static actions to the tray menu
	_trayMenu->addAction(_settingsAction);
#ifdef _WIN32
	_trayMenu->addAction(_autorunAction);
#endif

	// Bottom inline row: Suspend/Resume toggle, Restart, Quit
	{
		auto* bottomWidget = new QWidget();
		auto* hLayout = new QHBoxLayout(bottomWidget);
		hLayout->setContentsMargins(12, 6, 12, 6);
		hLayout->setSpacing(8);
		bottomWidget->setFixedHeight(38);

		QColor ic = _darkTheme ? Qt::white : Qt::black;

		// Suspend / Resume toggle
		_suspendResumeBtn = new QToolButton();
	_suspendResumeBtn->setIcon(recoloredIcon(":/suspend.svg", ic, 20));
	_suspendResumeBtn->setToolTip(tr("Suspend"));
	_suspendResumeBtn->setCheckable(true);
	_suspendResumeBtn->setIconSize(QSize(20, 20));
		_suspendResumeBtn->setFixedWidth(38);
		_suspendResumeBtn->setStyleSheet(
			"QToolButton { border: none; background: transparent; border-radius: 4px; }"
			"QToolButton:hover { background-color: #3d6db5; }"
		);
		hLayout->addWidget(_suspendResumeBtn, 0, Qt::AlignVCenter);

		connect(_suspendResumeBtn, &QToolButton::clicked, [this]() {
			QSharedPointer<Hyperion> hyperion;
			if (auto mgr = _instanceManagerWeak.toStrongRef())
			{
				hyperion = mgr->getHyperionInstance(_firstInstanceNumber);
			}
			if (hyperion.isNull())
				return;

			// isChecked() returns the post-auto-toggle state.
			// Checked  = user just clicked Suspend → disable
			// Unchecked = user just clicked Resume → enable
			bool enable = !_suspendResumeBtn->isChecked();
			emit hyperion->compStateChangeRequest(hyperion::COMP_LEDDEVICE, enable);
		});
		connect(_suspendResumeBtn, &QToolButton::toggled, [this, ic](bool checked) {
			_suspendResumeBtn->setIcon(recoloredIcon(checked ? ":/resume.svg" : ":/suspend.svg", ic, 20));
			_suspendResumeBtn->setToolTip(checked ? tr("Resume") : tr("Suspend"));
		});

		// Restart
		auto* restartBtn = new QToolButton();
restartBtn->setIcon(recoloredIcon(":/restart.svg", ic, 20));
	restartBtn->setToolTip(tr("Restart"));
	restartBtn->setIconSize(QSize(20, 20));
		restartBtn->setFixedWidth(38);
		restartBtn->setStyleSheet(
			"QToolButton { border: none; background: transparent; border-radius: 4px; }"
			"QToolButton:hover { background-color: #3d6db5; }"
		);
		hLayout->addWidget(restartBtn, 0, Qt::AlignVCenter);

		connect(restartBtn, &QToolButton::clicked, [this]() {
			emit signalEvent(Event::Restart);
		});

		// Quit
		auto* quitBtn = new QToolButton();
quitBtn->setIcon(recoloredIcon(":/quit.svg", ic, 20));
	quitBtn->setToolTip(tr("Quit"));
	quitBtn->setIconSize(QSize(20, 20));
		quitBtn->setFixedWidth(38);
		quitBtn->setStyleSheet(
			"QToolButton { border: none; background: transparent; border-radius: 4px; }"
			"QToolButton:hover { background-color: #3d6db5; }"
		);
		hLayout->addWidget(quitBtn, 0, Qt::AlignVCenter);

		connect(quitBtn, &QToolButton::clicked, []() {
			QApplication::quit();
		});

		_bottomActionsRow = new QWidgetAction(this);
		_bottomActionsRow->setDefaultWidget(bottomWidget);
		_trayMenu->addAction(_bottomActionsRow);
	}

	setContextMenu(_trayMenu);

	connect(_trayMenu, &QMenu::aboutToShow, this, &SysTray::updateStartupSourceIndicator);
}

void SysTray::setupConnections()
{
	WebServer const * webserver = _hyperiond->getWebServerInstance();
	_webPort = webserver->getPort();
	connect(webserver, &WebServer::portChanged, this, &SysTray::onWebserverPortChanged);
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		connect(mgr.get(), &HyperionIManager::instanceStateChanged, this, &SysTray::handleInstanceStateChange);
	}
	connect(this, &SysTray::signalEvent, EventHandler::getInstance().get(), &EventHandler::handleEvent);
}

QAction *SysTray::createAction(const QString &text, const QString &iconPath, const std::function<void()> &method)
{
	QColor ic = _darkTheme ? Qt::white : Qt::black;
	QIcon icon = recoloredIcon(iconPath, ic, 18);

	auto* btn = new QToolButton();
	btn->setIcon(icon);
	btn->setText(QStringLiteral("  ") + text);
	btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	btn->setIconSize(QSize(18, 18));
	btn->setAutoRaise(true);
	btn->setCursor(Qt::PointingHandCursor);
	btn->setStyleSheet(
		"QToolButton { border: none; text-align: left; padding: 4px 16px 4px 16px; font-size: 11pt; }"
		"QToolButton:hover { background-color: rgba(128,128,128,64); }"
	);
	QObject::connect(btn, &QToolButton::clicked, this, method);

	auto* action = new QWidgetAction(this);
	action->setDefaultWidget(btn);
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
		QColor selectedColor = QColorDialog::getColor (getInitialDialogColor(instance), nullptr, tr("Select Color"));
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
		emit hyperion->clear(100);
	}
}

void SysTray::handleInstanceStarted(quint8 instance)
{
	// Check if the instance already exists
	if (_instanceMenus.contains(instance))
	{
		return;
	}

	// Get instance name
	QString instanceName;
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		instanceName = mgr->getInstanceName(instance);
	}

	// First instance: horizontal icon buttons directly in main menu
	if (!_firstInstanceAction)
	{
		_firstInstanceNumber = instance;

#if defined(ENABLE_EFFECTENGINE)
		_firstEffectsMenu = new QMenu(tr("Effects"));
		_firstEffectsMenu->setObjectName("effectsMenu");

		const QList<EffectDefinition> effectsDefinitions = EffectFileHandler::getInstance()->getEffects();
		for (const auto& effect : effectsDefinitions)
		{
			QAction* effectAction = _firstEffectsMenu->addAction(effect.name);
			connect(effectAction, &QAction::triggered, [this, instance, effectName = effect.name]() {
				setEffect(instance, effectName);
			});
		}
		if (auto eff = EffectFileHandler::getInstance())
		{
			connect(eff.get(), &EffectFileHandler::effectListChanged, this, &SysTray::onEffectListChanged);
		}
#endif

		QColor ic = _darkTheme ? Qt::white : Qt::black;

		auto* btnWidget = new QWidget();
		auto* hLayout = new QHBoxLayout(btnWidget);
		hLayout->setContentsMargins(12, 6, 12, 6);
		hLayout->setSpacing(8);
		btnWidget->setFixedHeight(47);

		auto makeBtnGroup = [&](const QIcon& icon, const QString& tip, bool checkable = false) -> std::tuple<QToolButton*, QFrame*> {
			auto* container = new QWidget();
			container->setFixedWidth(38);
			auto* col = new QVBoxLayout(container);
			col->setContentsMargins(0, 0, 0, 0);
			col->setSpacing(1);

			auto* btn = new QToolButton();
			btn->setIcon(icon);
			btn->setToolTip(tip);
			if (checkable) btn->setCheckable(true);
			btn->setIconSize(QSize(20, 20));
			btn->setFixedWidth(38);
			btn->setStyleSheet(
				"QToolButton { border: none; background: transparent; border-radius: 4px; }"
				"QToolButton:hover { background-color: #3d6db5; }"
			);
			col->addWidget(btn, 0, Qt::AlignHCenter);

			auto* ind = new QFrame();
			ind->setFixedSize(38, 4);
			ind->setStyleSheet("background-color: transparent; border-radius: 2px;");
			col->addWidget(ind, 0, Qt::AlignHCenter);

			hLayout->addWidget(container, 0, Qt::AlignVCenter);
			return {btn, ind};
		};

		auto [colorBtn, colorInd] = makeBtnGroup(recoloredIcon(":/color.svg", ic, 20), tr("Color"));
		_colorIndicator = colorInd;

#if defined(ENABLE_EFFECTENGINE)
		auto [effectsBtn, effectsInd] = makeBtnGroup(recoloredIcon(":/effects.svg", ic, 20), tr("Effects"), true);
		effectsBtn->setPopupMode(QToolButton::InstantPopup);
		effectsBtn->setMenu(_firstEffectsMenu);
		_effectsIndicator = effectsInd;
#else
		QToolButton* effectsBtn = nullptr;
		QFrame* effectsInd = nullptr;
#endif

		auto [clearBtn, clearInd] = makeBtnGroup(recoloredIcon(":/clear.svg", ic, 20), tr("Clear"));

		// Color clicked
		connect(colorBtn, &QToolButton::clicked, [this, colorBtn, effectsBtn, effectsInd, instance, colorInd]() {
			QColor selectedColor = QColorDialog::getColor(getInitialDialogColor(instance), nullptr, tr("Select Color"));
			if (selectedColor.isValid())
			{
				_lastColor = selectedColor;
				setColor(instance, selectedColor);
				colorInd->setStyleSheet(QString(
					"background-color: %1; border-radius: 2px;"
				).arg(selectedColor.name()));
				if (effectsBtn) {
					effectsBtn->setChecked(false);
					effectsInd->setStyleSheet(
						"background-color: transparent; border-radius: 2px;"
					);
				}
			}
		});

		// Effects triggered
#if defined(ENABLE_EFFECTENGINE)
		connect(_firstEffectsMenu, &QMenu::triggered, [effectsBtn, colorInd, effectsInd](QAction*) {
			effectsBtn->setChecked(true);
			colorInd->setStyleSheet(
				"background-color: transparent; border-radius: 2px;"
			);
			effectsInd->setStyleSheet(
				"background-color: #32cd32; border-radius: 2px;"
			);
		});
#endif

		// Clear clicked
		connect(clearBtn, &QToolButton::clicked, [this, colorInd, effectsBtn, effectsInd, instance]() {
			clearSource(instance);
			colorInd->setStyleSheet(
				"background-color: transparent; border-radius: 2px;"
			);
			if (effectsBtn) {
				effectsBtn->setChecked(false);
				effectsInd->setStyleSheet(
					"background-color: transparent; border-radius: 2px;"
				);
			}
		});

		_firstInstanceAction = new QWidgetAction(this);
		_firstInstanceAction->setDefaultWidget(btnWidget);
		_trayMenu->insertAction(_settingsAction, _firstInstanceAction);

		updateStartupSourceIndicator();
	}
	else
	{
		// Subsequent instances get submenus
		auto* instanceMenu = new QMenu(instanceName);

		QAction *colorAction = createAction(tr("&Color"), ":/color.svg", [this, instance]() {
			showColorDialog(instance);
		});
		instanceMenu->addAction(colorAction);

#if defined(ENABLE_EFFECTENGINE)
		const QList<EffectDefinition> effectsDefinitions = EffectFileHandler::getInstance()->getEffects();
		if (!effectsDefinitions.empty())
		{
			auto* effectsMenu = new QMenu(tr("Effects"), instanceMenu);
			effectsMenu->setObjectName("effectsMenu");
			instanceMenu->addMenu(effectsMenu);

			for (const auto& effect : effectsDefinitions)
			{
				QAction* effectAction = effectsMenu->addAction(effect.name);
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

		_trayMenu->insertMenu(_bottomActionsRow, instanceMenu);
		_instanceMenus[instance] = instanceMenu;
	}

	// Keep suspend button in sync with async component state changes
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		if (auto hyperion = mgr->getHyperionInstance(instance))
		{
			connect(hyperion.get(), &Hyperion::isSetNewComponentState, this, [this](hyperion::Components comp, bool) {
				if (comp == hyperion::COMP_LEDDEVICE)
					syncSuspendButton();
			});
		}
	}
}

void SysTray::handleInstanceStopped(quint8 instance)
{
	// Check if the first instance (inline buttons) is stopping
	if (_firstInstanceAction && instance == _firstInstanceNumber)
	{
		_trayMenu->removeAction(_firstInstanceAction);
		delete _firstInstanceAction;
		_firstInstanceAction = nullptr;
		_colorIndicator = nullptr;
		_effectsIndicator = nullptr;

#if defined(ENABLE_EFFECTENGINE)
		delete _firstEffectsMenu;
		_firstEffectsMenu = nullptr;
#endif
		return;
	}

	// Check if the instance menu exists
	if (!_instanceMenus.contains(instance))
		return;

	// Remove the menu for this instance
	QMenu *instanceMenu = _instanceMenus.take(instance);
	_trayMenu->removeAction(instanceMenu->menuAction());

	// Delete the menu to free memory
	delete instanceMenu;
}

QColor SysTray::getInitialDialogColor(quint8 instance) const
{
	QSharedPointer<Hyperion> hyperion;
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		hyperion = mgr->getHyperionInstance(instance);
	}
	if (!hyperion.isNull())
	{
		int prio = hyperion->getCurrentPriority();
		PriorityMuxer::InputInfo info = hyperion->getPriorityInfo(prio);
		if (info.componentId == hyperion::COMP_COLOR && !info.ledColors.isEmpty())
		{
			const ColorRgb& c = info.ledColors.first();
			return QColor(c.red, c.green, c.blue);
		}
	}
	return _lastColor;
}

void SysTray::syncSuspendButton()
{
	QSharedPointer<Hyperion> hyperion;
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		hyperion = mgr->getHyperionInstance(_firstInstanceNumber);
	}
	if (hyperion.isNull() || !_suspendResumeBtn)
		return;

	bool isSuspended = !hyperion->isComponentEnabled(hyperion::COMP_LEDDEVICE);
	_suspendResumeBtn->setChecked(isSuspended);
}

void SysTray::updateStartupSourceIndicator()
{
	// Reset both indicators
	if (_colorIndicator)
		_colorIndicator->setStyleSheet("background-color: transparent; border-radius: 2px;");
	if (_effectsIndicator)
		_effectsIndicator->setStyleSheet("background-color: transparent; border-radius: 2px;");

	QSharedPointer<Hyperion> hyperion;
	if (auto mgr = _instanceManagerWeak.toStrongRef())
	{
		hyperion = mgr->getHyperionInstance(_firstInstanceNumber);
	}
	if (hyperion.isNull())
		return;

	syncSuspendButton();

	// Reflect the currently active source from the muxer
	int prio = hyperion->getCurrentPriority();
	PriorityMuxer::InputInfo info = hyperion->getPriorityInfo(prio);
	if (info.componentId == hyperion::COMP_COLOR && !info.ledColors.isEmpty())
	{
		const ColorRgb& c = info.ledColors.first();
		QColor activeColor(c.red, c.green, c.blue);
		if (_colorIndicator)
			_colorIndicator->setStyleSheet(
				QString("background-color: %1; border-radius: 2px;").arg(activeColor.name())
			);
	}
#if defined(ENABLE_EFFECTENGINE)
	else if (info.componentId == hyperion::COMP_EFFECT && _effectsIndicator)
	{
		_effectsIndicator->setStyleSheet(
			"background-color: #32cd32; border-radius: 2px;"
		);
	}
#endif
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

	// Update inline effects menu for the first instance
	if (_firstEffectsMenu)
	{
		_firstEffectsMenu->clear();
		quint8 instance = _firstInstanceNumber;
		for (const auto& effect : effectsDefinitions)
		{
			QAction* effectAction = _firstEffectsMenu->addAction(effect.name);
			connect(effectAction, &QAction::triggered, [this, instance, effectName = effect.name]() {
				setEffect(instance, effectName);
			});
		}
	}

	// Update submenu effects for other instances
	for (auto it = _instanceMenus.begin(); it != _instanceMenus.end(); ++it)
	{
		QMenu* instanceMenu = it.value();
		quint8 instanceNumber = it.key();

		QMenu* effectsMenu = instanceMenu->findChild<QMenu*>("effectsMenu");

		if (effectsMenu)
		{
			// Clear existing effects
			effectsMenu->clear();

			// Re-add the updated list of effects
			for (const auto& effect : effectsDefinitions)
			{
				QAction* effectAction = effectsMenu->addAction(effect.name);
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
	bool enabled = (reg.value("Hyperion", 0).toString() == QApplication::applicationFilePath().replace('/', '\\'));
	auto* wa = qobject_cast<QWidgetAction*>(_autorunAction);
	if (wa)
	{
		QToolButton* btn = qobject_cast<QToolButton*>(wa->defaultWidget());
		if (btn)
		{
			btn->setText(QStringLiteral("  ") + (!enabled ? tr("&Disable autostart") : tr("&Enable autostart")));
		}
	}
	return enabled;
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
