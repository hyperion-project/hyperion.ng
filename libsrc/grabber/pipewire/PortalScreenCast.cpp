#include "PortalScreenCast.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <grabber/GrabberTrace.h>


namespace {
const QString PORTAL_SERVICE = QStringLiteral("org.freedesktop.portal.Desktop");
const QString PORTAL_PATH = QStringLiteral("/org/freedesktop/portal/desktop");
const QString SCREENCAST_IFACE = QStringLiteral("org.freedesktop.portal.ScreenCast");
const QString REQUEST_IFACE = QStringLiteral("org.freedesktop.portal.Request");
const QString SESSION_IFACE = QStringLiteral("org.freedesktop.portal.Session");

constexpr uint SOURCE_MONITOR = 1;
constexpr uint CURSOR_HIDDEN = 1;
constexpr uint PERSIST_UNTIL_REVOKED = 2;
constexpr int CALL_TIMEOUT_MS = 10000;
}

PortalScreenCast::PortalScreenCast(const QString& tokenFile, QObject* parent)
	: QObject(parent)
	, _tokenFile(tokenFile)
{
	TRACK_SCOPE() << "Creating PortalScreenCast instance";
}

PortalScreenCast::~PortalScreenCast()
{
	TRACK_SCOPE();
	close();
}

QString PortalScreenCast::readToken() const
{
	QFile file(_tokenFile);
	if (!file.open(QIODevice::ReadOnly))
	{
		return {};
	}

	qCDebug(grabber_screen_flow) << "Read token from" << file.fileName();
	return QString::fromUtf8(file.readAll()).trimmed();
}

void PortalScreenCast::saveToken(const QString& token) const
{
	qCDebug(grabber_screen_flow) << "Saving token to" << _tokenFile;
	if (token.isEmpty())
	{
		return;
	}
	QDir().mkpath(QFileInfo(_tokenFile).absolutePath());
	QSaveFile file(_tokenFile);   // atomic: a crash mid-write cannot leave a truncated token
	if (file.open(QIODevice::WriteOnly))
	{
		qCDebug(grabber_screen_flow) << "Writing token to" << _tokenFile;
		file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
		file.write(token.toUtf8());
		file.commit();
	}
	else
	{
		qCWarning(grabber_screen_flow) << "[desktop-portal] Failed to save token to" << _tokenFile;
	}
}

void PortalScreenCast::fail(const QString& reason, bool userCancelled)
{
	_step = Step::Idle;
	emit failed(reason, userCancelled);
}

// Issues a portal method that answers asynchronously through a Request.Response signal.
// The signal path is derived from our bus name and handle token, so we subscribe before calling.
bool PortalScreenCast::callRequest(const QString& method, const QVariantList& args, const QString& handleToken)
{
	QDBusConnection bus = QDBusConnection::sessionBus();

	QString sender = bus.baseService().mid(1);
	sender.replace('.', '_');
	_requestPath = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, handleToken);

	if (!bus.connect(PORTAL_SERVICE, _requestPath, REQUEST_IFACE, QStringLiteral("Response"),
					 this, SLOT(onResponse(uint, QVariantMap))))
	{
		fail(QStringLiteral("Cannot subscribe to the portal response of %1").arg(method));
		return false;
	}

	// Asynchronous: a portal that is slow to activate must not stall the daemon's main loop. The
	// answer that matters arrives as the Response signal; the call's own reply only reports errors.
	QDBusMessage msg = QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_PATH, SCREENCAST_IFACE, method);
	msg.setArguments(args);
	auto* watcher = new QDBusPendingCallWatcher(bus.asyncCall(msg, CALL_TIMEOUT_MS), this);
	connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, method, watcher] {
		const QDBusPendingReply<QDBusObjectPath> reply = *watcher;
		watcher->deleteLater();
		if (reply.isError())
		{
			QDBusConnection::sessionBus().disconnect(PORTAL_SERVICE, _requestPath, REQUEST_IFACE, QStringLiteral("Response"),
													 this, SLOT(onResponse(uint, QVariantMap)));
			fail(QStringLiteral("%1 failed: %2").arg(method, reply.error().message()));
		}
	});
	return true;
}

void PortalScreenCast::start()
{
	qCDebug(grabber_screen_flow) << "Starting portal screen cast";
	if (_step != Step::Idle)
	{
		qCDebug(grabber_screen_flow) << "Portal screen cast already in progress, ignoring start request";
		return;
	}

	if (!QDBusConnection::sessionBus().isConnected())
	{
		fail(QStringLiteral("Cannot connect to the D-Bus session bus"));
		return;
	}

	_step = Step::CreateSession;
	const QString token = QStringLiteral("hyperion_%1").arg(++_counter);
	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;
	options[QStringLiteral("session_handle_token")] = QStringLiteral("hyperion_session_%1").arg(_counter);

	callRequest(QStringLiteral("CreateSession"), {options}, token);
}

void PortalScreenCast::onResponse(uint response, const QVariantMap& results)
{
	QDBusConnection bus = QDBusConnection::sessionBus();
	bus.disconnect(PORTAL_SERVICE, _requestPath, REQUEST_IFACE, QStringLiteral("Response"),
				   this, SLOT(onResponse(uint, QVariantMap)));

	if (response != 0)
	{
		// 1 = cancelled by the user, 2 = ended some other way
		fail(response == 1 ? QStringLiteral("The screen sharing dialog was cancelled")
						   : QStringLiteral("The portal request failed (code %1)").arg(response),
			 response == 1);
		return;
	}

	switch (_step)
	{
	case Step::CreateSession:
	{
		qCDebug(grabber_screen_flow) << "Portal returned session handle:" << results.value(QStringLiteral("session_handle")).toString();
		_sessionHandle = results.value(QStringLiteral("session_handle")).toString();
		if (_sessionHandle.isEmpty())
		{
			fail(QStringLiteral("The portal returned no session handle"));
			return;
		}

		_step = Step::SelectSources;
		const QString token = QStringLiteral("hyperion_%1").arg(++_counter);
		QVariantMap options;
		options[QStringLiteral("handle_token")] = token;
		options[QStringLiteral("types")] = SOURCE_MONITOR;
		options[QStringLiteral("multiple")] = false;
		options[QStringLiteral("cursor_mode")] = CURSOR_HIDDEN;
		options[QStringLiteral("persist_mode")] = PERSIST_UNTIL_REVOKED;
		const QString restoreToken = readToken();
		if (!restoreToken.isEmpty())
		{
			options[QStringLiteral("restore_token")] = restoreToken;
		}
		callRequest(QStringLiteral("SelectSources"), {QVariant::fromValue(QDBusObjectPath(_sessionHandle)), options}, token);
		break;
	}
	case Step::SelectSources:
	{
		qCDebug(grabber_screen_flow) << "Portal returned to SelectSources step";
		_step = Step::Start;
		const QString token = QStringLiteral("hyperion_%1").arg(++_counter);
		QVariantMap options;
		options[QStringLiteral("handle_token")] = token;
		// the dialog (if any) appears here
		callRequest(QStringLiteral("Start"), {QVariant::fromValue(QDBusObjectPath(_sessionHandle)), QString(), options}, token);
		break;
	}
	case Step::Start:
	{
		qCDebug(grabber_screen_flow) << "Portal returned to Start step with results:" << results;
		QString token = results.value(QStringLiteral("restore_token")).toString();
		if (!token.isEmpty())
		{
			saveToken(token);
		}

		quint32 node = 0;
		bool found = false;
		const QDBusArgument streams = results.value(QStringLiteral("streams")).value<QDBusArgument>();
		streams.beginArray();
		while (!streams.atEnd() && !found)
		{
			streams.beginStructure();
			QVariantMap props;
			streams >> node >> props;
			streams.endStructure();
			found = true;
		}
		streams.endArray();

		if (!found)
		{
			fail(QStringLiteral("The portal returned no stream"));
			return;
		}

		QDBusMessage msg = QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_PATH, SCREENCAST_IFACE,
														 QStringLiteral("OpenPipeWireRemote"));
		msg.setArguments({QVariant::fromValue(QDBusObjectPath(_sessionHandle)), QVariantMap()});
		auto* watcher = new QDBusPendingCallWatcher(bus.asyncCall(msg, CALL_TIMEOUT_MS), this);
		connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, node, watcher] {
			const QDBusPendingReply<QDBusUnixFileDescriptor> reply = *watcher;
			watcher->deleteLater();
			if (reply.isError())
			{
				fail(QStringLiteral("OpenPipeWireRemote failed: %1").arg(reply.error().message()));
				return;
			}

			// the receiver duplicates the descriptor while handling the signal
			const QDBusUnixFileDescriptor fd = reply.value();
			_step = Step::Idle;
			emit started(fd.fileDescriptor(), node);
		});
		break;
	}
	case Step::Idle:
		break;
	}
}  // NOSONAR: Qt parent owns this; deleteLater() called in finished lambda

void PortalScreenCast::close()
{
	qCDebug(grabber_screen_flow) << "Closing portal screen cast";
	if (_sessionHandle.isEmpty())
	{
		return;
	}

	QDBusMessage msg = QDBusMessage::createMethodCall(PORTAL_SERVICE, _sessionHandle, SESSION_IFACE, QStringLiteral("Close"));
	QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
	_sessionHandle.clear();
	_step = Step::Idle;
}
