#pragma once

// Qt includes
#include <QThread>
#include <QJsonObject>
#include <QSize>
#include <QImage>
#include <QPainter>

// Hyperion includes
#include <utils/Components.h>
#include <utils/Image.h>

#include <atomic>

class Hyperion;
class Logger;

class Effect : public QThread
{
	Q_OBJECT

public:

	friend class EffectModule;

	Effect(Hyperion *hyperion
				, int priority
				, int timeout
				, const QString &script
				, const QString &name
				, const QJsonObject &args = QJsonObject()
				, const QString &imageData = ""
	);
	~Effect() override;

	void run() override;

	int getPriority() const { return _priority; }

	///
	/// @brief Set manual interruption to true,
	///        Note: DO NOT USE QThread::interruption!
	///
	void requestInterruption() { _interupt = true; }

	///
	/// @brief Check an interruption was requested.
	///        This can come from requestInterruption()
	///        or the effect's timeout expiring.
	///
	/// @return    The flag state
	///
	bool isInterruptionRequested();

	///
	/// @brief Get the remaining timeout, or indication it is endless
	///
	/// @return    The flag state
	///
	int getRemaining() const;


	QString getScript() const { return _script; }
	QString getName() const { return _name; }

	int getTimeout() const {return _timeout; }
	bool isEndless() const { return _isEndless; }

	QJsonObject getArgs() const { return _args; }

signals:
	void setInput(int priority, const std::vector<ColorRgb> &ledColors, int timeout_ms, bool clearEffect);
	void setInputImage(int priority, const Image<ColorRgb> &image, int timeout_ms, bool clearEffect);

private:
	void setModuleParameters();
	void addImage();

	Hyperion *_hyperion;

	const int _priority;

	const int _timeout;
	bool _isEndless;

	const QString _script;
	const QString _name;

	const QJsonObject _args;
	const QString _imageData;

	qint64 _endTime;

	/// Buffer for colorData
	QVector<ColorRgb> _colors;

	Logger *_log;
	// Reflects whenever this effects should interrupt (timeout or external request)
	std::atomic<bool> _interupt {};

	QSize           _imageSize;
	QImage          _image;
	QPainter       *_painter;
	QVector<QImage> _imageStack;
};
