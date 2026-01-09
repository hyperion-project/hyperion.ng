

// Platform-specific only
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dxgi1_2.h>
#include <d2d1_1.h>
#include <atlbase.h>
#include <comdef.h>

#include "grabber/dda/DDAGrabber.h"
#include <QDebug>
#include <QJsonDocument>

#include <utils/Logger.h>

#include <cmath>

// Logs a message along with the hex error HRESULT.
#define LOG_ERROR(hr, msg) Error(_log, msg ": 0x%x", hr)

// Checks if the HRESULT is an error, and if so, logs it and returns from the
// current function.
#define RETURN_IF_ERROR(hr, msg, returnValue)                                                                          \
	if (FAILED(hr))                                                                                                    \
	{                                                                                                                  \
		setInError(QString("%1: 0x%2").arg(msg).arg(hr));                                                              \
		return returnValue;                                                                                            \
	}

// Checks if the condition is false, and if so, logs an error and returns from
// the current function.
#define RET_CHECK(cond, returnValue)                                                                                   \
	if (!(cond))                                                                                                       \
	{                                                                                                                  \
		Error(_log, "Assertion failed: " #cond);                                                                       \
		return returnValue;                                                                                            \
	}

// Safely call IDXGIOutputDuplication::ReleaseFrame and swallow SEH that some drivers raise
static inline HRESULT SafeReleaseFrame(IDXGIOutputDuplication* dup)
{
	if (!dup) return E_POINTER;
#if defined(_MSC_VER)
	__try
	{
		return dup->ReleaseFrame();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return DXGI_ERROR_INVALID_CALL;
	}
#else
	return dup->ReleaseFrame();
#endif
}

class DDAGrabberImpl
{
public:
	CComPtr<ID3D11Device> device;
	CComPtr<ID3D11DeviceContext> deviceContext;
	CComPtr<IDXGIDevice> dxgiDevice;
	CComPtr<IDXGIAdapter> dxgiAdapter;
	CComPtr<IDXGIOutput1> dxgiOutput1;
	CComPtr<IDXGIOutputDuplication> desktopDuplication;

	CComPtr<ID3D11Texture2D> intermediateTexture;

	CComPtr<ID2D1Factory1>        d2dFactory;
	CComPtr<ID2D1Device>          d2dDevice;
	CComPtr<ID2D1DeviceContext>   d2dContext;
	CComPtr<ID3D11Texture2D>      d2dConvertedTexture; // Holds the B8G8R8A8 converted image

	// Pre-calculated members for performance
	CComPtr<ID2D1Bitmap1>     destBitmap;
	D2D1_MATRIX_3X2_F         orientationTransform{};
	D2D1_RECT_F               sourceRect{};
	D2D1_RECT_F               destRect{};

	int display = 0;

	DXGI_MODE_ROTATION desktopRotation = DXGI_MODE_ROTATION_IDENTITY;
	int desktopWidth = 0;
	int desktopHeight = 0;
	bool isFrameAcquired = false;
};

DDAGrabber::DDAGrabber(int display, int cropLeft, int cropRight, int cropTop, int cropBottom)
	: Grabber("GRABBER-DDA", cropLeft, cropRight, cropTop, cropBottom)
	, d(new DDAGrabberImpl)
{
	TRACK_SCOPE() << "Creating DDA grabber for display" << d->display;
	_useImageResampler = false;
	d->display = display;
}

DDAGrabber::~DDAGrabber()
{
	TRACK_SCOPE();
	if (d->isFrameAcquired && d->desktopDuplication)
	{
		SafeReleaseFrame(d->desktopDuplication);
		d->isFrameAcquired = false;
	}
}

bool DDAGrabber::setupDisplay()
{
	qCDebug(grabber_screen_flow) << "Setting up DDA grabber for display" << d->display;

	d->device.Release();
	d->deviceContext.Release();
	d->dxgiDevice.Release();
	d->dxgiAdapter.Release();
	d->dxgiOutput1.Release();
	d->desktopDuplication.Release();

	// Release D2D resources
	d->d2dFactory.Release();
	d->d2dDevice.Release();
	d->d2dContext.Release();
	d->d2dConvertedTexture.Release();
	d->destBitmap.Release();

	// Create a hardware device only (Desktop Duplication requires hardware adapter)
	UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	// If you want D3D debug messages, you can add the debug flag too
#ifdef QT_DEBUG
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr{ S_OK };
	hr = D3D11CreateDevice(
		nullptr,                 // Default adapter
		D3D_DRIVER_TYPE_HARDWARE,// Require hardware for DDA
		nullptr,                 // Software device (not used)
		createFlags,             // Flags
		nullptr,                 // Feature levels to attempt
		0,                       // Number of feature levels
		D3D11_SDK_VERSION,       // SDK version
		&d->device,              // Returned ID3D11Device
		nullptr,                 // Returned feature level
		&d->deviceContext        // Returned ID3D11DeviceContext
	);
	RETURN_IF_ERROR(hr, "CreateDevice failed", false);

	hr = d->device->QueryInterface(&d->dxgiDevice);
	RETURN_IF_ERROR(hr, "Failed to get DXGI device", false);

	D2D1_FACTORY_OPTIONS d2dOptions = {};
#ifdef QT_DEBUG
	d2dOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &d2dOptions, (void**)&d->d2dFactory);
	RETURN_IF_ERROR(hr, "Failed to create D2D1Factory", false);

	hr = d->d2dFactory->CreateDevice(d->dxgiDevice, &d->d2dDevice);
	RETURN_IF_ERROR(hr, "Failed to create D2D1Device", false);

	hr = d->d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d->d2dContext);
	RETURN_IF_ERROR(hr, "Failed to create D2D1DeviceContext", false);

	hr = d->dxgiDevice->GetAdapter(&d->dxgiAdapter);
	RETURN_IF_ERROR(hr, "Failed to get DXGI adapter", false);

	// Log adapter description and LUID for diagnostics
	if (grabber_screen_flow().isDebugEnabled())
	{
		DXGI_ADAPTER_DESC adapterDesc{};
		d->dxgiAdapter->GetDesc(&adapterDesc);
		qCDebug(grabber_screen_flow) << "DXGI Adapter:" << QString::fromWCharArray(adapterDesc.Description)
			<< ", VendorId:" << adapterDesc.VendorId << ", DeviceId:" << adapterDesc.DeviceId
			<< ", SubSysId:" << adapterDesc.SubSysId << ", Revision:" << adapterDesc.Revision
			<< ", LUID:" << (qulonglong)adapterDesc.AdapterLuid.HighPart << ":" << (qulonglong)adapterDesc.AdapterLuid.LowPart;

		// Log session info (console session id)
		DWORD sessionId = WTSGetActiveConsoleSessionId();
		qCDebug(grabber_screen_flow) << "Active console session id:" << sessionId;
	}

	return true;
}

bool DDAGrabber::restartCapture()
{
	if (_isDeviceInError)
	{
		Error(_log, "Cannot restart capture, device is in error state");
		return false;
	}

	qCDebug(grabber_screen_flow) << "Restarting capture for display" << d->display;

	if (d->dxgiAdapter == nullptr)
	{
		if (!setupDisplay())
		{
			setInError("restartCapture - Setting up the display failed");
			return false;
		}
		// Continue with capture setup after setting up the display successfully
	}

	HRESULT hr{ S_OK };

	CComPtr<IDXGIOutput> output;
	hr = d->dxgiAdapter->EnumOutputs(d->display, &output);
	RETURN_IF_ERROR(hr, "Failed to get output", false);

	// Forcefully drop previous duplication state after ACCESS_LOST/SESSION_DISCONNECTED
	// so we never "reuse" an invalid duplication/output.
	d->desktopDuplication.Release();
	d->dxgiOutput1.Release();

	// Always re-query a fresh IDXGIOutput1 from the current output
	{
		qCDebug(grabber_screen_flow) << "Creating new output for display" << d->display;
		CComPtr<IDXGIOutput1> output1;
		hr = output->QueryInterface(&output1);
		RETURN_IF_ERROR(hr, "Failed to get output1", false);
		d->dxgiOutput1 = output1;
		qCDebug(grabber_screen_flow) << "Created new output for display" << d->display;
	}

	DXGI_OUTPUT_DESC desc;
	hr = output->GetDesc(&desc);
	RETURN_IF_ERROR(hr, "Failed to get output description", false);

	d->desktopWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
	d->desktopHeight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;

	if (_cropLeft + _cropRight >= d->desktopWidth || _cropTop + _cropBottom >= d->desktopHeight)
	{
		Error(_log, "Invalid cropping values which exceed the screen size. Cropping disabled.");
		_cropLeft = _cropRight = _cropTop = _cropBottom = 0;
	}

	int croppedWidth = d->desktopWidth - (_cropLeft + _cropRight);
	int croppedHeight = d->desktopHeight - (_cropTop + _cropBottom);

	int finalWidth = qMax(1, croppedWidth / _pixelDecimation);
	int	finalHeight = qMax(1, croppedHeight / _pixelDecimation);

	// Now duplication is null, so this block will recreate the duplication (and resources if needed)
	if (desc.Rotation != d->desktopRotation || finalWidth != _width || finalHeight != _height || d->desktopDuplication == nullptr)
	{
		Debug(_log, "New capture size or rotation detected. Creating Desktop Duplication for display %d", d->display);

		_width = finalWidth;
		_height = finalHeight;
		d->desktopRotation = desc.Rotation;

		// Recreate desktop duplication
		hr = d->dxgiOutput1->DuplicateOutput(d->device, &d->desktopDuplication);
		if (FAILED(hr))
		{
			LOG_ERROR(hr, "DuplicateOutput failed");
			// If device is lost/removed, recreate device and try again once
			if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
			{
				qCDebug(grabber_screen_flow) << "Device removed/reset. Recreating D3D device and retrying duplication.";
				if (!setupDisplay())
				{
					RETURN_IF_ERROR(hr, "Failed to recreate device after removal", false);
				}
				// Need to re-enum output after device recreation
				CComPtr<IDXGIOutput> outputRetry;
				hr = d->dxgiAdapter->EnumOutputs(d->display, &outputRetry);
				RETURN_IF_ERROR(hr, "Failed to get output (retry)", false);
				CComPtr<IDXGIOutput1> output1Retry;
				hr = outputRetry->QueryInterface(&output1Retry);
				RETURN_IF_ERROR(hr, "Failed to get output1 (retry)", false);
				d->dxgiOutput1 = output1Retry;
				// Retry DuplicateOutput
				hr = d->dxgiOutput1->DuplicateOutput(d->device, &d->desktopDuplication);
			}

			if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
			{
				Error(_log, "DuplicateOutput failed: Another application or system component is currently using the desktop interface.");
			}
			else if (hr == DXGI_ERROR_SESSION_DISCONNECTED)
			{
				Error(_log, "DuplicateOutput failed: Desktop session is currently locked or disconnected. Will retry.");
			}

			RETURN_IF_ERROR(hr, "Failed to create desktop duplication interface", false);
		}

		// 1. Create the final GPU texture and staging texture.
		qCDebug(grabber_screen_flow) << "Creating final-sized GPU resources [" << _width << "x" << _height << "]";
		D3D11_TEXTURE2D_DESC finalDesc = {};
		finalDesc.Width = _width;
		finalDesc.Height = _height;
		finalDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		finalDesc.MipLevels = 1;
		finalDesc.ArraySize = 1;
		finalDesc.SampleDesc.Count = 1;
		finalDesc.Usage = D3D11_USAGE_DEFAULT;
		finalDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

		d->d2dConvertedTexture.Release();
		hr = d->device->CreateTexture2D(&finalDesc, nullptr, &d->d2dConvertedTexture);
		RETURN_IF_ERROR(hr, "Failed to create final GPU texture", false);

		// Recreate the CPU staging texture to match
		finalDesc.Usage = D3D11_USAGE_STAGING;
		finalDesc.BindFlags = 0;
		finalDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		d->intermediateTexture.Release();
		hr = d->device->CreateTexture2D(&finalDesc, nullptr, &d->intermediateTexture);
		RETURN_IF_ERROR(hr, "Failed to create final staging texture", false);

		// 2. Pre-create the destination D2D bitmap
		CComPtr<IDXGISurface> destSurface;
		d->d2dConvertedTexture->QueryInterface(&destSurface);
		d->destBitmap.Release();
		d->d2dContext->CreateBitmapFromDxgiSurface(destSurface, nullptr, &d->destBitmap);

		// Determine rotated dimensions for D3D
		bool swapDimensions = d->desktopRotation == DXGI_MODE_ROTATION_ROTATE90 || d->desktopRotation == DXGI_MODE_ROTATION_ROTATE270;
		// This should reference the final output size, not the full desktop size
		int renderTargetWidth = swapDimensions ? _height : _width;
		int renderTargetHeight = swapDimensions ? _width : _height;

		// 3. Pre-calculate the transformation matrix for D2D
		switch (d->desktopRotation)
		{
		case DXGI_MODE_ROTATION_ROTATE90:
			d->orientationTransform =
				D2D1::Matrix3x2F::Rotation(90.0f) *
				D2D1::Matrix3x2F::Translation(static_cast<float>(renderTargetHeight), 0.0f);
			break;
		case DXGI_MODE_ROTATION_ROTATE180:
			d->orientationTransform =
				D2D1::Matrix3x2F::Rotation(180.0f) *
				D2D1::Matrix3x2F::Translation(static_cast<float>(renderTargetWidth), static_cast<float>(renderTargetHeight));
			break;
		case DXGI_MODE_ROTATION_ROTATE270:
			d->orientationTransform =
				D2D1::Matrix3x2F::Rotation(270.0f) *
				D2D1::Matrix3x2F::Translation(0.0f, static_cast<float>(renderTargetWidth));
			break;
		default:
			d->orientationTransform = D2D1::Matrix3x2F::Identity();
			break;
		}

		// 4. Pre-calculate crop and destination rectangles

		// Source D3D picture captured is always not rotated
		int sourceWidth = swapDimensions ? d->desktopHeight : d->desktopWidth;
		int sourceHeight = swapDimensions ? d->desktopWidth : d->desktopHeight;

		D3D11_BOX cropBox{};
		computeCropBox(sourceWidth, sourceHeight, cropBox);

		d->sourceRect = D2D1::RectF(
			static_cast<float>(cropBox.left),
			static_cast<float>(cropBox.top),
			static_cast<float>(cropBox.right),
			static_cast<float>(cropBox.bottom)
		);

		d->destRect = D2D1::RectF(0.0f, 0.0f, static_cast<float>(renderTargetWidth), static_cast<float>(renderTargetHeight));

		Debug(_log, "Display capture set up - Desktop size: %dx%d, Rotation=%d, cropping=%d,%d,%d,%d, decimation=%d, output image size=%dx%d",
			d->desktopWidth, d->desktopHeight, d->desktopRotation, _cropLeft, _cropTop, _cropRight, _cropBottom, _pixelDecimation,
			_width, _height);
	}
	else
	{
		// Should not happen since we forced Release(), but keep fallback log
		qCDebug(grabber_screen_flow) << "Reusing existing output for display" << d->display;
	}

	return true;
}

bool DDAGrabber::resetDeviceAndCapture()
{
	qCDebug(grabber_screen_flow) << "Resetting device and capture for display" << d->display;
	resetInError();
	return setupDisplay() && restartCapture();
}

int DDAGrabber::grabFrame(Image<ColorRgb>& image)
{
	if (_isDeviceInError)
	{
		Error(_log, "Cannot grab frame, device is in error state");
		return -1;
	}

	if (!_isEnabled)
	{
		qDebug() << "DDAGrabber: Capture is disabled";
		return -1;
	}

	if (!d->desktopDuplication)
	{
		qCDebug(grabber_screen_capture) << "Desktop duplication not available (yet)";
		return -1;
	}

	if (d->isFrameAcquired)
	{
		HRESULT hrRelease = SafeReleaseFrame(d->desktopDuplication);
		d->isFrameAcquired = false;
		if (FAILED(hrRelease))
		{
			if (grabber_screen_capture().isDebugEnabled())
			{
				if (hrRelease == DXGI_ERROR_ACCESS_LOST)
				{
					qCDebug(grabber_screen_capture) << "Failed to release frame- DXGI_ERROR_ACCESS_LOST";
				}
			}
			// Even if release fails, we try to continue. The state is reset.
		}
	}

	HRESULT hr{ S_OK };
	CComPtr<IDXGIResource> desktopResource;
	DXGI_OUTDUPL_FRAME_INFO frameInfo = {};
	hr = d->desktopDuplication->AcquireNextFrame(20, &frameInfo, &desktopResource);
	if (SUCCEEDED(hr))
	{
		d->isFrameAcquired = true;
	}

	// --- Error Handling ---
	if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL || hr == DXGI_ERROR_SESSION_DISCONNECTED || hr == DXGI_ERROR_ACCESS_DENIED || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
	{
		if (d->isFrameAcquired)
		{
			SafeReleaseFrame(d->desktopDuplication);
			d->isFrameAcquired = false;
		}

		QString errorText;
		switch (hr)
		{
		case DXGI_ERROR_ACCESS_LOST:
			// Happens, if the graphics device is removed/reset or orientation changed
			qCDebug(grabber_screen_flow) << "Access lost - DXGI_ERROR_ACCESS_LOST. Restarting capture.";
			Error(_log, "Access lost to desktop duplication, attempting to reset device and capture... (DXGI_ERROR_ACCESS_LOST)");
			restartCapture();
			return -1;

			break;
		case DXGI_ERROR_INVALID_CALL:
			// Happens, if a screen is locked or disconnected
			qCDebug(grabber_screen_flow) << "Access lost - DXGI_ERROR_INVALID_CALL";
			return -1;
			break;
		case DXGI_ERROR_ACCESS_DENIED:
			errorText = "Access denied : The application does not have the required permissions to capture the desktop";
			break;
		case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
			errorText = "Access lost: Another application or system component is currently using the desktop interface.";
			break;
		case DXGI_ERROR_SESSION_DISCONNECTED:
			errorText = "Access lost: Desktop session is currently locked or disconnected. Will retry.";
			break;
		default:
			errorText = "Access lost : Unknown error";
		}

		if (grabber_screen_flow().isDebugEnabled())
		{
			// Log session/desktop info for diagnostics
			DWORD sessionId = WTSGetActiveConsoleSessionId();
			qCDebug(grabber_screen_flow) << "Active console session id:" << sessionId;
			DXGI_OUTDUPL_DESC duplDesc{};
			if (d->desktopDuplication)
			{
				d->desktopDuplication->GetDesc(&duplDesc);
				qCDebug(grabber_screen_flow) << "Duplication Desc: ModeDesc(" << duplDesc.ModeDesc.Width << "x" << duplDesc.ModeDesc.Height << ") Rot=" << duplDesc.Rotation;
			}
		}

		RETURN_IF_ERROR(hr, errorText, -1);
	}

	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		return -1;
	}

	if (FAILED(hr))
	{
		qCDebug(grabber_screen_flow) << "Failed to acquire next frame: " << hr;
		return -1;
	}

	qCDebug(grabber_screen_capture) << "FrameInfo: Accumulated=" << frameInfo.AccumulatedFrames
		<< ", PointerVisible=" << (int)frameInfo.PointerPosition.Visible
		<< ", LastPresentTime=" << (qulonglong)frameInfo.LastPresentTime.QuadPart
		<< ", LastMouseUpdateTime=" << (qulonglong)frameInfo.LastMouseUpdateTime.QuadPart
		<< " Capture setup: width:" << _width << ", height:" << _height << ", pixelDecimation:" << _pixelDecimation;

	CComPtr<ID3D11Texture2D> sourceTexture;
	hr = desktopResource->QueryInterface(&sourceTexture);
	if (FAILED(hr))
	{
		if (d->isFrameAcquired)
		{
			SafeReleaseFrame(d->desktopDuplication);
			d->isFrameAcquired = false;
		}
		RETURN_IF_ERROR(hr, "Failed to get 2D texture from resource", -1);
	}

	// Use the D2D pipeline to handle any transformation or format conversion.
	CComPtr<IDXGISurface> sourceSurface;
	sourceTexture->QueryInterface(&sourceSurface);
	CComPtr<ID2D1Bitmap1> sourceBitmap;
	d->d2dContext->CreateBitmapFromDxgiSurface(sourceSurface, nullptr, &sourceBitmap);

	// Draw to our B8G8R8A8 target, performing all transforms (using pre-calculated members)
	d->d2dContext->SetTarget(d->destBitmap);
	d->d2dContext->BeginDraw();
	d->d2dContext->SetTransform(d->orientationTransform);
	d->d2dContext->DrawBitmap(sourceBitmap, d->destRect, 1.0f, D2D1_INTERPOLATION_MODE_LINEAR, d->sourceRect);
	hr = d->d2dContext->EndDraw();
	d->d2dContext->SetTarget(nullptr);

	// Copy the D2D result to the staging texture
	d->deviceContext->CopyResource(d->intermediateTexture, d->d2dConvertedTexture);
	d->deviceContext->Flush();

	if (FAILED(hr))
	{
		setInError(QString("D2D DrawBitmap failed: 0x%1").arg(hr));
		return -1;
	}

	// Map and copy to user image
	D3D11_MAPPED_SUBRESOURCE mapped;
	hr = d->deviceContext->Map(d->intermediateTexture, 0, D3D11_MAP_READ, 0, &mapped);
	RETURN_IF_ERROR(hr, "Failed to map final texture", -1);

	// CPU pixel copy from BGRA to output RGB image
	ColorRgb* destPtr = image.memptr();
	const uint8_t* srcPtr = static_cast<const uint8_t*>(mapped.pData);

	for (int y = 0; y < _height; ++y)
	{
		// Set pointers to the start of the current row
		const uint32_t* srcRowPtr = reinterpret_cast<const uint32_t*>(srcPtr + y * mapped.RowPitch);
		ColorRgb* destRowPtr = destPtr + y * _width;

		for (int x = 0; x < _width; ++x)
		{
			const uint8_t* bgra = reinterpret_cast<const uint8_t*>(srcRowPtr);
			destRowPtr->red = bgra[2];
			destRowPtr->green = bgra[1];
			destRowPtr->blue = bgra[0];

			// Move to the next pixel
			srcRowPtr++;
			destRowPtr++;
		}
	}

	d->deviceContext->Unmap(d->intermediateTexture, 0);

	// The frame is intentionally held until the next grabFrame call for performance.
	return 0;
}

void DDAGrabber::computeCropBox(int sourceWidth, int sourceHeight, D3D11_BOX& box) const
{
	switch (d->desktopRotation) {
	case DXGI_MODE_ROTATION_ROTATE90:
		box.left = _cropTop;
		box.right = sourceWidth - _cropBottom;
		box.top = _cropRight;
		box.bottom = sourceHeight - _cropLeft;
		break;
	case DXGI_MODE_ROTATION_ROTATE270:
		box.left = _cropBottom;
		box.right = sourceWidth - _cropTop;
		box.top = _cropLeft;
		box.bottom = sourceHeight - _cropRight;
		break;
	case DXGI_MODE_ROTATION_ROTATE180:
		box.left = _cropRight;
		box.right = sourceWidth - _cropLeft;
		box.top = _cropBottom;
		box.bottom = sourceHeight - _cropTop;
		break;
	case DXGI_MODE_ROTATION_IDENTITY:
	default:
		box.left = _cropLeft;
		box.right = sourceWidth - _cropRight;
		box.top = _cropTop;
		box.bottom = sourceHeight - _cropBottom;
		break;
	}

	box.front = 0;
	box.back = 1;
}

bool DDAGrabber::setDisplayIndex(int index)
{
	qCDebug(grabber_screen_flow) << "Setting display index to" << index << ", current is" << d->display;
	bool rc = true;
	if (d->display != index)
	{
		d->display = index;
		rc = resetDeviceAndCapture();
	}
	return rc;
}
QJsonObject DDAGrabber::discover(const QJsonObject& params)
{
	QJsonObject inputsDiscovered;
	if (isAvailable(false) && setupDisplay())
	{
		HRESULT hr = S_OK;

		// Enumerate through the outputs.
		QJsonArray videoInputs;
		for (int i = 0;; ++i)
		{
			CComPtr<IDXGIOutput> output;
			hr = d->dxgiAdapter->EnumOutputs(i, &output);
			if (!output || !SUCCEEDED(hr))
			{
				break;
			}

			// Get the output description.
			DXGI_OUTPUT_DESC desc;
			hr = output->GetDesc(&desc);
			if (FAILED(hr))
			{
				Error(_log, "Failed to get output description");
				continue;
			}

			// Add it to the JSON.
			const int width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
			const int height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;

			qDebug() << "Found video input" << i << "with name" << QString::fromWCharArray(desc.DeviceName)
				<< "and size" << width << "x" << height;

			videoInputs.append(QJsonObject{
				{"inputIdx", i},
				{"name", QString::fromWCharArray(desc.DeviceName)},
				{"formats",
				 QJsonArray{
					 QJsonObject{
						 {"resolutions",
						  QJsonArray{
							  QJsonObject{
								  {"width", width},
								  {"height", height},
								  {"fps", QJsonArray{1, 5, 10, 15, 20, 25, 30, 40, 50, 60, 120, 144}},
							  },
						  }},
					 },
				 }},
				});
		}

		inputsDiscovered["video_inputs"] = videoInputs;
		if (!videoInputs.isEmpty())
		{
			inputsDiscovered["device"] = "dda";
			inputsDiscovered["device_name"] = "DXGI DDA";
			inputsDiscovered["type"] = "screen";
			inputsDiscovered["default"] = QJsonObject{
				{"video_input",
				 QJsonObject{
					 {"inputIdx", 0},
					 {"resolution",
					  QJsonObject{
						  {"fps", 60},
					  }},
				 }},
			};
		}
	}

	if (inputsDiscovered.isEmpty())
	{
		qCDebug(grabber_screen_properties) << "No displays found to capture from!";
	}
	return inputsDiscovered;
}
