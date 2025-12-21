#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

#include <QThread>
#include <QMetaObject>
#include <QObject>
#include <QDebug>

/**
 * @brief Safely and synchronously stops a worker object running in a QThread and waits for the thread to exit.
 *
 * This function implements the robust Qt shutdown pattern:
 * 1. Connects the worker's 'stopped' signal to the thread's 'quit' slot.
 * 2. Invokes the worker's 'stop' slot via the event queue.
 * 3. Blocks the calling thread using QThread::wait() until the worker thread exits.
 *
 * @tparam Worker The type of the worker object (must inherit QObject and have the Q_OBJECT macro).
 * @tparam StopSignal A pointer-to-member for the worker's signal emitted when stopping is complete (e.g., &Worker::stopped).
 * @tparam StopSlot A pointer-to-member for the worker's slot that initiates the stop procedure (e.g., &Worker::stop).
 * * @param workerPtr Pointer to the worker object (use data() for QScopedPointer, or get() for std::unique_ptr).
 * @param threadPtr Pointer to the QThread object.
 * @param stoppedSignal The worker's signal that confirms cleanup is complete.
 * @param stopSlot The worker's slot to initiate shutdown.
 * @param timeoutMs Maximum time to wait for the thread to finish in milliseconds.
 */
template <typename Worker, typename StopSignal, typename StopSlot>
void safeShutdownThread(
	Worker* workerPtr,
	QThread* threadPtr,
	StopSignal stoppedSignal,
	StopSlot stopSlot,
	long timeoutMs = 5000)
{
	// Ensure essential components exist and the thread is active
	if (!workerPtr || !threadPtr || !threadPtr->isRunning()) {
		if (threadPtr) {
			qDebug() << "Thread is not running or pointers are null. Skipping safe shutdown.";
		}
		return;
	}

	// 1. Connect the worker's 'stopped' signal to the thread's 'quit' slot.
	// Use Qt::DirectConnection for reliable cross-thread signaling here.
	QObject::connect(workerPtr, stoppedSignal,
		threadPtr, &QThread::quit,
		Qt::DirectConnection);

	// 2. Request the worker to stop by sending the stop slot to its event loop.
	QMetaObject::invokeMethod(workerPtr, stopSlot, Qt::QueuedConnection);

	// 3. Block and wait for the thread to finish gracefully.
	if (!threadPtr->wait(timeoutMs)) {
		qWarning().nospace() << "Thread (" << threadPtr << ") failed to exit gracefully after "
			<< timeoutMs << "ms. Forced termination imminent.";
		// The thread will eventually be killed by the QThread destructor if the QScopedPointer
		// is destroyed, but this log indicates a potential hang in the worker's cleanup.
	}
}

/**
 * @brief Performs an immediate stop and wait on a QThread.
 * * This is suitable ONLY for simple worker threads that do not rely on a queued
 * stop() slot being processed, or where the worker object is manually deleted
 * before this call. Generally, safeShutdownThread is preferred.
 *
 * @param threadPtr Pointer to the QThread object.
 */
inline void stopThreadImmediate(QThread* threadPtr)
{
	if (threadPtr && threadPtr->isRunning()) {
		threadPtr->quit();
		threadPtr->wait();
	}
}

#endif // THREAD_UTILS_H
