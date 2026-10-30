#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

///
/// @brief xdg-desktop-portal ScreenCast handshake (CreateSession, SelectSources, Start,
/// OpenPipeWireRemote) over D-Bus.
///
/// Requests a persistent session and stores the restore token the portal hands back, so the
/// next start does not need the user to confirm the dialog again.
///
class PortalScreenCast : public QObject
{
	Q_OBJECT

public:
	explicit PortalScreenCast(const QString& tokenFile, QObject* parent = nullptr);
	~PortalScreenCast() override;

	/// Starts the handshake, asynchronous. Emits started() or failed().
	void start();

	/// Closes the portal session, which also ends the stream.
	void close();

signals:
	/// @param pipewireFd  connection to the PipeWire remote, owned by the receiver (dup it)
	void started(int pipewireFd, quint32 nodeId);
	/// @param userCancelled  the user dismissed the dialog (do not ask again right away)
	void failed(const QString& reason, bool userCancelled);

private slots:
	void onResponse(uint response, const QVariantMap& results);

private:
	enum class Step { Idle, CreateSession, SelectSources, Start };

	bool callRequest(const QString& method, const QVariantList& args, const QString& handleToken);
	QString readToken() const;
	void saveToken(const QString& token) const;
	void fail(const QString& reason, bool userCancelled = false);

	QString _tokenFile;
	QString _sessionHandle;
	QString _requestPath;
	Step _step = Step::Idle;
	int _counter = 0;
};
