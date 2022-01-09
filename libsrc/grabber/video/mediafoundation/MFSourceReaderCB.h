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
#include <comdef.h>

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
#ifdef HAVE_TURBO_JPEG
	if (IsEqualGUID(guid, MFVideoFormat_MJPG)) return  PixelFormat::MJPEG;
#endif
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
		, _isBusy(false)
		, _transform(nullptr)
		, _pixelformat(PixelFormat::NO_CHANGE)
	{
		// Initialize critical section.
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
		DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample)
	{
		EnterCriticalSection(&_critsec);
		_isBusy = true;

		if (_grabber->_sourceReader == nullptr)
		{
			_isBusy = false;
			LeaveCriticalSection(&_critsec);
			return S_OK;
		}

		if (dwStreamFlags & MF_SOURCE_READERF_STREAMTICK)
		{
			Debug(_grabber->_log, "Skipping stream gap");
			LeaveCriticalSection(&_critsec);
			_grabber->_sourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
			return S_OK;
		}

		if (dwStreamFlags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED)
		{
			IMFMediaType* type = nullptr;
			GUID format;
			_grabber->_sourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, MF_SOURCE_READER_CURRENT_TYPE_INDEX, &type);
			type->GetGUID(MF_MT_SUBTYPE, &format);
			Debug(_grabber->_log, "Native media type changed");
			InitializeVideoEncoder(type, GetPixelFormatForGuid(format));
			SAFE_RELEASE(type);
		}

		if (dwStreamFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
		{
			IMFMediaType* type = nullptr;
			GUID format;
			_grabber->_sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &type);
			type->GetGUID(MF_MT_SUBTYPE, &format);
			Debug(_grabber->_log, "Current media type changed");
			InitializeVideoEncoder(type, GetPixelFormatForGuid(format));
			SAFE_RELEASE(type);
		}

		// Variables declaration
		IMFMediaBuffer* buffer = nullptr;
		BYTE* data = nullptr;
		DWORD maxLength = 0, currentLength = 0;



		if (FAILED(hrStatus))
		{
			_hrStatus = hrStatus;
			Error(_grabber->_log, "0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		if (!pSample)
		{
			Error(_grabber->_log, "Media sample is empty");
			goto done;
		}

#ifdef HAVE_TURBO_JPEG
		if (_pixelformat != PixelFormat::MJPEG && _pixelformat != PixelFormat::BGR24 && _pixelformat != PixelFormat::NO_CHANGE)
#else
		if (_pixelformat != PixelFormat::BGR24 && _pixelformat != PixelFormat::NO_CHANGE)
#endif
			pSample = TransformSample(_transform, pSample);

		_hrStatus = pSample->ConvertToContiguousBuffer(&buffer);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Buffer conversion failed => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}


		_hrStatus = buffer->Lock(&data, &maxLength, &currentLength);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Access to the buffer memory failed => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		_grabber->receive_image(data, currentLength);

		_hrStatus = buffer->Unlock();
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Unlocking the buffer memory failed => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
		}

	done:
		SAFE_RELEASE(buffer);

		if (MF_SOURCE_READERF_ENDOFSTREAM & dwStreamFlags)
			_bEOS = TRUE; // Reached the end of the stream.

#ifdef HAVE_TURBO_JPEG
		if (_pixelformat != PixelFormat::MJPEG && _pixelformat != PixelFormat::BGR24 && _pixelformat != PixelFormat::NO_CHANGE)
#else
		if (_pixelformat != PixelFormat::BGR24 && _pixelformat != PixelFormat::NO_CHANGE)
#endif
			SAFE_RELEASE(pSample);

		_isBusy = false;
		LeaveCriticalSection(&_critsec);
		return _hrStatus;
	}

	HRESULT InitializeVideoEncoder(IMFMediaType* type, PixelFormat format)
	{
		_pixelformat = format;
#ifdef HAVE_TURBO_JPEG
		if (format == PixelFormat::MJPEG || format == PixelFormat::BGR24 || format == PixelFormat::NO_CHANGE)
#else
		if (format == PixelFormat::BGR24 || format == PixelFormat::NO_CHANGE)
#endif
			return S_OK;

		// Variable declaration
		IMFMediaType* output = nullptr;
		DWORD mftStatus = 0;
		QString error = "";

		// Create instance of IMFTransform interface pointer as CColorConvertDMO
		_hrStatus = CoCreateInstance(CLSID_CColorConvertDMO, nullptr, CLSCTX_INPROC_SERVER, IID_IMFTransform, (void**)&_transform);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Creation of the Color Converter failed => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		// Set input type as media type of our input stream
		_hrStatus = _transform->SetInputType(0, type, 0);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Setting the input media type failed => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		// Create new media type
		_hrStatus = MFCreateMediaType(&output);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Creating a new media type failed => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		// Copy all attributes from input type to output media type
		_hrStatus = type->CopyAllItems(output);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Copying of all attributes from input to output media type failed => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		UINT32 width, height;
		UINT32 numerator, denominator;

		// Fill the missing attributes

		if (FAILED(output->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video)) ||
			FAILED(output->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24)) ||
			FAILED(output->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, TRUE)) ||
			FAILED(output->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE)) ||
			FAILED(output->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive)) ||
			FAILED(MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height)) ||
			FAILED(MFSetAttributeSize(output, MF_MT_FRAME_SIZE, width, height)) ||
			FAILED(MFGetAttributeRatio(type, MF_MT_FRAME_RATE, &numerator, &denominator)) ||
			FAILED(MFSetAttributeRatio(output, MF_MT_PIXEL_ASPECT_RATIO, 1, 1)))
		{
			Error(_grabber->_log, "Setting output media type attributes failed");
			goto done;
		}

		// Set transform output type
		_hrStatus = _transform->SetOutputType(0, output, 0);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Setting the output media type failed => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		// Check if encoder parameters set properly
		_hrStatus = _transform->GetInputStatus(0, &mftStatus);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Failed to query the input stream for more data => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		if (MFT_INPUT_STATUS_ACCEPT_DATA == mftStatus)
		{
			// Notify the transform we are about to begin streaming data
			if (FAILED(_transform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0)) ||
				FAILED(_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0)) ||
				FAILED(_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0)))
			{
				Error(_grabber->_log, "Failed to begin streaming data");
			}
		}

	done:
		SAFE_RELEASE(output);
		return _hrStatus;
	}

	BOOL isBusy()
	{
		EnterCriticalSection(&_critsec);
		BOOL result = _isBusy;
		LeaveCriticalSection(&_critsec);

		return result;
	}

	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent*) { return S_OK; }
	STDMETHODIMP OnFlush(DWORD) { return S_OK; }

private:
	virtual ~SourceReaderCB()
	{
		if (_transform)
		{
			_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
			_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, 0);
		}

		SAFE_RELEASE(_transform);

		// Delete critical section.
		DeleteCriticalSection(&_critsec);
	}

	IMFSample* TransformSample(IMFTransform* transform, IMFSample* in_sample)
	{
		IMFSample* result = nullptr;
		IMFMediaBuffer* out_buffer = nullptr;
		MFT_OUTPUT_DATA_BUFFER outputDataBuffer = { 0 };
		DWORD status = 0;

		// Process the input sample
		_hrStatus = transform->ProcessInput(0, in_sample, 0);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Failed to process the input sample => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		// Gets the buffer demand for the output stream
		MFT_OUTPUT_STREAM_INFO streamInfo;
		_hrStatus = transform->GetOutputStreamInfo(0, &streamInfo);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Failed to retrieve buffer requirement for output current => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		// Create an output media buffer
		_hrStatus = MFCreateMemoryBuffer(streamInfo.cbSize, &out_buffer);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Failed to create an output media buffer => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		// Create an empty media sample
		_hrStatus = MFCreateSample(&result);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Failed to create an empty media sampl => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		// Add the output media buffer to the media sample
		_hrStatus = result->AddBuffer(out_buffer);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Failed to add the output media buffer to the media sample => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
			goto done;
		}

		// Create the output buffer structure
		memset(&outputDataBuffer, 0, sizeof outputDataBuffer);
		outputDataBuffer.dwStreamID = 0;
		outputDataBuffer.dwStatus = 0;
		outputDataBuffer.pEvents = nullptr;
		outputDataBuffer.pSample = result;

		// Generate the output sample
		_hrStatus = transform->ProcessOutput(0, 1, &outputDataBuffer, &status);
		if (FAILED(_hrStatus))
		{
			Error(_grabber->_log, "Failed to generate the output sample => 0x%08x: %s", _hrStatus, std::system_category().message(_hrStatus).c_str());
		}
		else
		{
			SAFE_RELEASE(out_buffer);
			return result;
		}

	done:
		SAFE_RELEASE(out_buffer);
		return nullptr;
	}

private:
	long				_nRefCount;
	CRITICAL_SECTION	_critsec;
	MFGrabber*			_grabber;
	BOOL				_bEOS;
	HRESULT				_hrStatus;
	IMFTransform*		_transform;
	PixelFormat			_pixelformat;
	std::atomic<bool>	_isBusy;
};
