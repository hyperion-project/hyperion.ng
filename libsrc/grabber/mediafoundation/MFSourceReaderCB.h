#pragma once

#include <mfapi.h>
#include <mftransform.h>
#include <dmo.h>
#include <wmcodecdsp.h>
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
#pragma comment (lib, "strmiids.lib")
#pragma comment (lib, "wmcodecdspuuid.lib")

#include <grabber/MFGrabber.h>

#define SAFE_RELEASE(x) if(x) { x->Release(); x = nullptr; }

// Need more supported formats? Visit https://docs.microsoft.com/en-us/windows/win32/medfound/colorconverter
static PixelFormat GetPixelFormatForGuid(const GUID guid)
{
	if (IsEqualGUID(guid, MFVideoFormat_RGB32)) return PixelFormat::RGB32;
	if (IsEqualGUID(guid, MFVideoFormat_RGB24)) return PixelFormat::BGR24;
	if (IsEqualGUID(guid, MFVideoFormat_YUY2)) return PixelFormat::YUYV;
	if (IsEqualGUID(guid, MFVideoFormat_UYVY)) return PixelFormat::UYVY;
	if (IsEqualGUID(guid, MFVideoFormat_MJPG)) return  PixelFormat::MJPEG;
	if (IsEqualGUID(guid, MFVideoFormat_NV12)) return  PixelFormat::NV12;
	if (IsEqualGUID(guid, MFVideoFormat_I420)) return  PixelFormat::I420;
	return PixelFormat::NO_CHANGE;
};

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
	STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD /*dwStreamIndex*/,
		DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample)
	{
		EnterCriticalSection(&_critsec);

        if(dwStreamFlags & MF_SOURCE_READERF_STREAMTICK)
		{
			Debug(_grabber->_log, "Skipping stream gap");
			LeaveCriticalSection(&_critsec);
			_grabber->_sourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
			return S_OK;
        }

		if (dwStreamFlags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED)
		{
			IMFMediaType *type = nullptr;
			GUID format;
			_grabber->_sourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, MF_SOURCE_READER_CURRENT_TYPE_INDEX, &type);
			type->GetGUID(MF_MT_SUBTYPE, &format);
			Debug(_grabber->_log, "Native media type changed");
			InitializeVideoEncoder(type, GetPixelFormatForGuid(format));
			SAFE_RELEASE(type);
		}

		if (dwStreamFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
		{
			IMFMediaType *type = nullptr;
			GUID format;
			_grabber->_sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &type);
			type->GetGUID(MF_MT_SUBTYPE, &format);
			Debug(_grabber->_log, "Current media type changed");
			InitializeVideoEncoder(type, GetPixelFormatForGuid(format));
			SAFE_RELEASE(type);
		}

		if (SUCCEEDED(hrStatus))
		{
			QString error = "";

			if (pSample)
			{
				//variables declaration
				IMFMediaBuffer* buffer = nullptr, *mediaBuffer = nullptr;
				IMFSample* sampleOut = nullptr;

				if(_pixelformat == PixelFormat::MJPEG || _pixelformat == PixelFormat::NO_CHANGE)
				{
					hrStatus = pSample->ConvertToContiguousBuffer(&buffer);
					if (SUCCEEDED(hrStatus))
					{
						BYTE* data = nullptr;
						DWORD maxLength = 0, currentLength = 0;

						hrStatus = buffer->Lock(&data, &maxLength, &currentLength);
						if(SUCCEEDED(hrStatus))
						{
							_grabber->receive_image(data,currentLength);
							buffer->Unlock();
						}
						else
							error = QString("buffer->Lock failed => %1").arg(hrStatus);
					}
					else
						error = QString("pSample->ConvertToContiguousBuffer failed => %1").arg(hrStatus);
				}
				else
				{
					// Send our input sample to the transform
					_transform->ProcessInput(0, pSample, 0);

					MFT_OUTPUT_STREAM_INFO streamInfo;
					hrStatus = _transform->GetOutputStreamInfo(0, &streamInfo);
					if (SUCCEEDED(hrStatus))
					{
						hrStatus = MFCreateMemoryBuffer(streamInfo.cbSize, &buffer);
						if (SUCCEEDED(hrStatus))
						{
							hrStatus = MFCreateSample(&sampleOut);
							if (SUCCEEDED(hrStatus))
							{
								hrStatus = sampleOut->AddBuffer(buffer);
								if (SUCCEEDED(hrStatus))
								{
									MFT_OUTPUT_DATA_BUFFER outputDataBuffer = {0};
									memset(&outputDataBuffer, 0, sizeof outputDataBuffer);
									outputDataBuffer.dwStreamID = 0;
									outputDataBuffer.dwStatus = 0;
									outputDataBuffer.pEvents = nullptr;
									outputDataBuffer.pSample = sampleOut;

									DWORD status = 0;
									hrStatus = _transform->ProcessOutput(0, 1, &outputDataBuffer, &status);
									if (hrStatus == MF_E_TRANSFORM_NEED_MORE_INPUT)
									{
										SAFE_RELEASE(sampleOut);
										SAFE_RELEASE(buffer);
										return S_OK;
									}

									hrStatus = sampleOut->ConvertToContiguousBuffer(&mediaBuffer);
									if (SUCCEEDED(hrStatus))
									{
										BYTE* data = nullptr;
										DWORD currentLength = 0;

										hrStatus = mediaBuffer->Lock(&data, 0, &currentLength);
										if(SUCCEEDED(hrStatus))
										{
											_grabber->receive_image(data, currentLength);
											mediaBuffer->Unlock();
										}
										else
											error = QString("mediaBuffer->Lock failed => %1").arg(hrStatus);
									}
									else
										error = QString("sampleOut->ConvertToContiguousBuffer failed => %1").arg(hrStatus);
								}
								else
									error = QString("AddBuffer failed %1").arg(hrStatus);
							}
							else
								error = QString("MFCreateSample failed %1").arg(hrStatus);
						}
						else
							error = QString("MFCreateMemoryBuffer failed %1").arg(hrStatus);
					}
					else
						error = QString("GetOutputStreamInfo failed %1").arg(hrStatus);
				}

				SAFE_RELEASE(buffer);
				SAFE_RELEASE(mediaBuffer);
				SAFE_RELEASE(sampleOut);
			}
			else
				error = "pSample is NULL";

			if (!error.isEmpty())
				Error(_grabber->_log, "%s", QSTRING_CSTR(error));
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

	HRESULT SourceReaderCB::InitializeVideoEncoder(IMFMediaType* type, PixelFormat format)
	{
		_pixelformat = format;
		if (format == PixelFormat::MJPEG || format == PixelFormat::NO_CHANGE)
			return S_OK;

		// Variable declaration
		IMFMediaType *output = nullptr;
		DWORD mftStatus = 0;
		QString error = "";

		// Create instance of IMFTransform interface pointer as CColorConvertDMO
		if (SUCCEEDED(CoCreateInstance(CLSID_CColorConvertDMO, nullptr, CLSCTX_INPROC_SERVER, IID_IMFTransform, (void**)&_transform)))
		{
			// Set input type as media type of our input stream
			_hrStatus = _transform->SetInputType(0, type, 0);
			if (SUCCEEDED(_hrStatus))
			{
				// Create new media type
				_hrStatus = MFCreateMediaType(&output);
				if (SUCCEEDED(_hrStatus))
				{
					// Copy data from input type to output type
					_hrStatus = type->CopyAllItems(output);
					if (SUCCEEDED(_hrStatus))
					{
						UINT32 width, height;
						UINT32 numerator, denominator;

						// Fill the missing attributes
						output->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
						output->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24);
						output->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, TRUE);
						output->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
						output->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
						MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height);
						MFSetAttributeSize(output, MF_MT_FRAME_SIZE, width, height);
						MFGetAttributeRatio(type, MF_MT_FRAME_RATE, &numerator, &denominator);
						MFSetAttributeRatio(output, MF_MT_FRAME_RATE, numerator, denominator);
						MFSetAttributeRatio(output, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

						if (SUCCEEDED(_hrStatus))
						{
							// Set transform output type
							_hrStatus = _transform->SetOutputType(0, output, 0);
							if (SUCCEEDED(_hrStatus))
							{
								// Check if encoder parameters set properly
								_hrStatus = _transform->GetInputStatus(0, &mftStatus);
								if (SUCCEEDED(_hrStatus))
								{
									if (MFT_INPUT_STATUS_ACCEPT_DATA == mftStatus)
									{
										// Notify the transform we are about to begin streaming data
										if (FAILED(_transform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0)) ||
											FAILED(_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0)) ||
											FAILED(_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0)))
										{
											error = QString("ProcessMessage failed %1").arg(_hrStatus);
										}
									}
									else
									{
										_hrStatus = S_FALSE;
										error = QString("GetInputStatus failed %1").arg(_hrStatus);
									}
								}
								else
									error = QString("GetInputStatus failed %1").arg(_hrStatus);
							}
							else
								error = QString("SetOutputType failed %1").arg(_hrStatus);
						}
						else
							error = QString("Can not set output attributes %1").arg(_hrStatus);
					}
					else
						error = QString("CopyAllItems failed %1").arg(_hrStatus);
				}
				else
					error = QString("MFCreateMediaType failed %1").arg(_hrStatus);
			}
			else
				error = QString("SetInputType failed %1").arg(_hrStatus);
		}
		else
			error = QString("CoCreateInstance failed %1").arg(_hrStatus);

		if (!error.isEmpty())
			Error(_grabber->_log, "%s", QSTRING_CSTR(error));

		SAFE_RELEASE(output);
		return _hrStatus;
	}

	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *) { return S_OK; }
	STDMETHODIMP OnFlush(DWORD) { return S_OK; }

private:
	virtual ~SourceReaderCB()
	{
		_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
		_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, 0);
		SAFE_RELEASE(_transform);
	}

	void NotifyError(HRESULT hr) { Error(_grabber->_log, "Source Reader error"); }

private:
	long				_nRefCount;
	CRITICAL_SECTION	_critsec;
	MFGrabber*			_grabber;
	BOOL				_bEOS;
	HRESULT				_hrStatus;
	IMFTransform*		_transform;
	PixelFormat			_pixelformat;
};
