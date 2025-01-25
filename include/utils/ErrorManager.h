#ifndef ERRORMANAGER_H
#define ERRORMANAGER_H

#include <QObject>

class ErrorManager : public QObject
{
	Q_OBJECT

public:
	explicit ErrorManager(QObject *parent = nullptr) : QObject(parent) {}

signals:
	void errorOccurred(QString error);
};

#endif // ERRORMANAGER_H
