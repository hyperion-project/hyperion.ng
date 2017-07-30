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
	SysTray(HyperionDaemon *hyperiond, quint16 webPort);
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
	quint16          _webPort;
	Hyperion        *_hyperion;
};
