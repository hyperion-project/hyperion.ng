#pragma once

#include <utils/Logger.h>

class QUdpSocket;

///
/// @brief The SSDP Server sends and receives (parses) SSDP requests
///
class SSDPServer : public QObject
{
	Q_OBJECT

public:
	///
	/// @brief Construct the server, listen on default ssdp address/port with multicast
	/// @param parent  The parent object
	///
	SSDPServer(QObject* parent = nullptr);
	~SSDPServer() override;

	///
	/// @brief Send an answer to mSearch requester
	/// @param st         the searchTarget
	/// @param senderIp   Ip address of the sender
	/// @param senderPort The port of the sender
	///
	void sendMSearchResponse(const QString& st, const QString& senderIp, quint16 senderPort);

	///
	/// @brief Send ByeBye notification (on SSDP stop) (repeated 3 times)
	/// @param st        Search target
	///
	void sendByeBye(const QString& st);

	///
	/// @brief Send a NOTIFY msg on SSDP startup to notify our presence (repeated 3 times)
	/// @param st        The search target
	///
	void sendAlive(const QString& st);

	///
	/// @brief Send a NOTIFY msg as ssdp:update to notify about changes
	/// @param st        The search target
	///
	void sendUpdate(const QString& st);

	///
	/// @brief Overwrite description address
	/// @param addr  new address
	///
	void setDescriptionAddress( const QString &addr ) { _descAddress = addr; }

	///
	/// @brief Get the description address
	/// @return addr  new address
	///
	virtual QString getDescriptionAddress() const { return _descAddress; }

	///
	/// @brief Set uuid
	/// @param uuid  The uuid
	///
	void setUuid( const QString &uuid ) { _uuid = uuid; }

	///
	/// @brief Get uuid
	/// @return uuid  The uuid
	///
	QString getUuid() const { return _uuid; }

public slots:
	///
	/// @brief Prepare server after thread start
	///
	void initServer();

	///
	/// @brief Start SSDP
	/// @return false if already running or bind failure
	///
	bool start();

	///
	/// @brief Stop SSDP
	///
	virtual void stop();
	
signals:
	///
	/// @brief Emits whenever a new SSDP search "man : ssdp:discover" is received along with the service type
	/// @param target  The ST service type
	/// @param mx      Answer with delay in s
	/// @param address The ip of the caller
	/// @param port    The port of the caller
	///
	void msearchRequestReceived( const QString &target,
								 const QString &mx,
								 const QString address,
								 quint16 port );

protected:

	virtual QString getInfo() const { return {}; };

	QSharedPointer<Logger> _log;

private:

	QScopedPointer<QUdpSocket> _udpSocket;

	QString _serverHeader;
	QString _uuid;
	QString _descAddress;
	bool _running;

private slots:
	void readPendingDatagrams();
};
