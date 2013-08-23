
#pragma once

// QT includes
#include <QTimer>

// Hyperion includes
#include <hyperion/Hyperion.h>

class RainbowBootSequence : public QObject
{
Q_OBJECT

public:
	RainbowBootSequence(Hyperion * hyperion);

	void start();

private slots:
	void update();

private:
	QTimer _timer;

	Hyperion * _hyperion;

	int _priority;

	std::vector<RgbColor> _ledColors;
	int _iterationCounter;
};

