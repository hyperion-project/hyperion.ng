#pragma once

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <mferror.h>
#include <strmif.h>

#pragma comment (lib, "ole32.lib")
#pragma comment (lib, "mf.lib")
#pragma comment (lib, "mfplat.lib")
#pragma comment (lib, "mfuuid.lib")
#pragma comment (lib, "mfreadwrite.lib")

#include <grabber/MFGrabber.h>

#define SAFE_RELEASE(x) if(x) { x->Release(); x = nullptr; }

class SourceReaderCB : public IMFSourceReaderCallback
{
public:
	SourceReaderCB(MFGrabber* grabber)
		: _nRefCount(1)
		, _grabber(grabber)
		, _bEOS(FALSE)
		, _hrStatus(S_OK)
	{
		InitializeCriticalSection(&_critsec);
	}

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv)
	{
		static const QITAB qit[] =
		{
			QITABENT(SourceReaderCB, IMFSourceReaderCallback),
			{ 0 },
		};
		return QISearch(this, qit, iid, ppv);
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&_nRefCount);
	}

	STDMETHODIMP_(ULONG) Release()
	{
		ULONG uCount = InterlockedDecrement(&_nRefCount);
		if (uCount == 0)
		{
			delete this;
		}
		return uCount;
	}

	// IMFSourceReaderCallback methods
	STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
		DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample)
	{
		EnterCriticalSection(&_critsec);

		if (SUCCEEDED(hrStatus))
		{
			QString error = "";
			bool frameSend = false;

			if (pSample)
			{
				IMFMediaBuffer* buffer;

				hrStatus = pSample->ConvertToContiguousBuffer(&buffer);
				if (SUCCEEDED(hrStatus))
				{
					BYTE* data = nullptr;
					DWORD maxLength = 0, currentLength = 0;

					hrStatus = buffer->Lock(&data, &maxLength, &currentLength);
					if(SUCCEEDED(hrStatus))
					{
						frameSend = true;
						_grabber->receive_image(data,currentLength,error);

						buffer->Unlock();
					}
					else
						error = QString("buffer->Lock failed => %1").arg(hrStatus);

				}
				else
					error = QString("pSample->ConvertToContiguousBuffer failed => %1").arg(hrStatus);

				SAFE_RELEASE(buffer);
			}
			else
				error = "pSample is NULL";

			if (!frameSend)
				_grabber->receive_image(NULL,0,error);
		}
		else
		{
			// Streaming error.
			NotifyError(hrStatus);
		}

		if (MF_SOURCE_READERF_ENDOFSTREAM & dwStreamFlags)
		{
			// Reached the end of the stream.
			_bEOS = TRUE;
		}
		_hrStatus = hrStatus;

		LeaveCriticalSection(&_critsec);
		return S_OK;
	}

	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *) { return S_OK; }
	STDMETHODIMP OnFlush(DWORD) { return S_OK; }

private:
	virtual ~SourceReaderCB() {}
	void NotifyError(HRESULT hr) { Error(_grabber->_log, "Source Reader error"); }

private:
	long             _nRefCount;
	CRITICAL_SECTION _critsec;
	MFGrabber*       _grabber;
	BOOL             _bEOS;
	HRESULT          _hrStatus;
};
