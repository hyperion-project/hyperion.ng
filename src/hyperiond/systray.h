#pragma once

#include <QSystemTrayIcon>
#include <QMenu>
#include <QWidget>
#include <QColorDialog>
#include <QCloseEvent>

#include <hyperion/Hyperion.h>

class HyperionDaemon;

class SysTray : public QWidget
{
	Q_OBJECT

public:
	SysTray(HyperionDaemon *hyperiond);
	~SysTray();


public slots:
	void showColorDialog();
	void setColor(const QColor & color);
	void closeEvent(QCloseEvent *event);
	void settings();
	void setEffect();
	void clearEfxColor();

private slots:
	void iconActivated(QSystemTrayIcon::ActivationReason reason);

	///
	/// @brief is called whenever the webserver changes the port
	///
	void webserverPortChanged(const quint16& port) { _webPort = port; };

private:
	void createTrayIcon();

	QAction *quitAction;
	QAction *startAction;
	QAction *stopAction;
	QAction *colorAction;
	QAction *settingsAction;
	QAction *clearAction;

	QSystemTrayIcon *_trayIcon;
	QMenu           *_trayIconMenu;
	QMenu           *_trayIconEfxMenu;
	QColorDialog     _colorDlg;
	HyperionDaemon  *_hyperiond;
	Hyperion        *_hyperion;
	quint16          _webPort;
};
