#pragma once

#include <QByteArray>
#include <QString>
#include <memory>

///
/// @brief Latest frame copied out of a PipeWire screen cast stream.
///
struct CapturedFrame
{
	QByteArray data;
	int width = 0;
	int height = 0;
	int stride = 0;
	bool bgrOrder = true;   ///< 4 bytes per pixel, B-G-R-x when true, R-G-B-x otherwise
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
	PipeWireFrameSource();
	~PipeWireFrameSource();

	/// @param fd  PipeWire remote, duplicated internally
	bool start(int fd, quint32 nodeId, QString& error);
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
