#pragma once

#include <memory>

#include <QByteArray>
#include <QString>

#include <utils/PixelFormat.h>

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

	/// @param fd      PipeWire remote, duplicated internally (portal mode)
	/// @param nodeId  Node to connect to (from the portal handshake)
	bool start(int fd, quint32 nodeId, QString& error);

	/// @brief Connect to the running PipeWire daemon directly (no portal FD needed).
	///        The stream is attached to the node whose "node.name" property matches
	///        @p targetNodeName (e.g. "gamescope").  PipeWire's session manager
	///        (WirePlumber) resolves the name to the actual node.
	/// @param targetNodeName  Value of the node's "node.name" PW property.
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
