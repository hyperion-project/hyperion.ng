#pragma once

#include <memory>

#include <QByteArray>
#include <QList>
#include <QString>

#include <utils/PixelFormat.h>

///
/// @brief Information about an active Gamescope video node in PipeWire.
///
struct GamescopeNodeInfo
{
	uint32_t id{0};
	QString name;
	QString displayName;
};

///
/// @brief Latest frame copied out of a PipeWire screen cast stream.
///
struct CapturedFrame
{
	QByteArray data;
	int width{0};
	int height{0};
	size_t stride{0};
	PixelFormat pixelFormat{PixelFormat::NO_CHANGE};
};

///
/// @brief Consumes a screen cast node from an already connected PipeWire remote.
///
/// Only shared-memory buffers are requested, so frames are linear and CPU readable, whatever
/// tiling or compression the compositor uses for scanout.
///
class PipeWireFrameSource
{

public:
	static constexpr int MAX_FPS = 240;

	PipeWireFrameSource();
	~PipeWireFrameSource();

	/// @brief Enumerate active Gamescope video nodes in the current PipeWire session.
	static QList<GamescopeNodeInfo> discoverGamescopeNodes();

	/// @param fd      PipeWire remote, duplicated internally (portal mode)
	/// @param nodeId  Node to connect to (from the portal handshake)
	bool start(int fd, quint32 nodeId, QString& error);

	/// @brief Connect to the running PipeWire daemon directly (no portal FD needed).
	/// @param targetNodeId    Specific node ID to connect to, or 0 for auto / targetNodeName.
	/// @param targetNodeName  Node name (e.g. "gamescope") used when targetNodeId is 0.
	bool startDirect(uint32_t targetNodeId, const QString& targetNodeName, QString& error);

	/// @brief Convenience overload targeting by node name.
	bool startDirect(const QString& targetNodeName, QString& error);

	void stop();

	/// Copies the most recent frame, false until the first one has arrived.
	bool latest(CapturedFrame& out);

	/// Frames are copied at most this often; the consumer cannot use more than its own rate.
	void setMinFrameInterval(int ms);

	/// True once the stream has ended or errored (e.g. sharing stopped from the desktop).
	bool hasFailed() const;

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};
