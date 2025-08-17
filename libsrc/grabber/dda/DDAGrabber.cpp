
// Platform-specific only
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dxgi1_2.h>
#include <d2d1_1.h>
#include <atlbase.h>

#include "grabber/dda/DDAGrabber.h"
#include <QDebug>
#include <QJsonDocument>

#include <utils/Logger.h>

#include <cmath>


// Constants
namespace {
	const bool verbose = true;
} //End of constants

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
};

DDAGrabber::DDAGrabber(int display, int cropLeft, int cropRight, int cropTop, int cropBottom)
	: Grabber("GRABBER-DDA", cropLeft, cropRight, cropTop, cropBottom)
	, d(new DDAGrabberImpl)
{
	_useImageResampler = false;
	d->display = display;
	qDebug() << "Creating DDA grabber for display" << d->display;
}

DDAGrabber::~DDAGrabber()
{
}

bool DDAGrabber::open()
{
	qDebug() << "Opening DDA grabber for display" << d->display;

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

	static const D3D_DRIVER_TYPE driverTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};

	// Add a variable for the creation flags
	UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	// If you want D3D debug messages, you can add the debug flag too
#ifdef QT_DEBUG
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr{ S_OK };
	for (auto driverType : driverTypes)
	{
		hr = D3D11CreateDevice(
			nullptr,                 // Adapter to use
			driverType,              // Driver type (since we specified adapter)
			nullptr,                 // Software device (not used)
			createFlags,             // Flags
			nullptr,                 // Feature levels to attempt
			0,                       // Number of feature levels
			D3D11_SDK_VERSION,       // SDK version
			&d->device,              // Returned ID3D11Device
			nullptr,                 // Returned feature level
			&d->deviceContext        // Returned ID3D11DeviceContext
		);
		if (SUCCEEDED(hr)) break;
	}
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

	return true;
}

bool DDAGrabber::restartCapture()
{
	if (_isDeviceInError)
	{
		Error(_log, "Cannot restart capture, device is in error state");
		return false;
	}

	qDebug() << "Restarting capture for display" << d->display;

	if (d->dxgiAdapter == nullptr)
	{
		if (!open())
		{
			setInError("restartCapture - Open failed");
			return false;
		}
		return false;
	}

	HRESULT hr{ S_OK };

	CComPtr<IDXGIOutput> output;
	hr = d->dxgiAdapter->EnumOutputs(d->display, &output);
	RETURN_IF_ERROR(hr, "Failed to get output", false);

	if (d->dxgiOutput1 == nullptr)
	{
		Debug(_log, "Creating new output for display %d", d->display);
		CComPtr<IDXGIOutput1> output1;
		hr = output->QueryInterface(&output1);
		RETURN_IF_ERROR(hr, "Failed to get output1", false);
		d->dxgiOutput1 = output1;
		Debug(_log, "Created new output for display %d", d->display);
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

	// Check if a full re-initialization is needed
	if (desc.Rotation != d->desktopRotation || finalWidth != _width || finalHeight != _height || d->desktopDuplication == nullptr)
	{
		Debug(_log, "New capture size or rotation detected. Creating Desktop Duplication for display %d", d->display);

		_width = finalWidth;
		_height = finalHeight;
		d->desktopRotation = desc.Rotation;

		// Recreate desktop duplication
		d->desktopDuplication.Release();
		hr = d->dxgiOutput1->DuplicateOutput(d->device, &d->desktopDuplication);
		RETURN_IF_ERROR(hr, "Failed to create desktop duplication interface", false);

		// 1. Create the final GPU texture and staging texture.
		Debug(_log, "Creating final-sized GPU resources [%dx%d]", _width, _height);
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
			d->orientationTransform =
				D2D1::Matrix3x2F::Identity();
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
		Debug(_log, "Reusing existing output for display %d", d->display);
	}

	return true;
}

bool DDAGrabber::resetDeviceAndCapture()
{
	Debug(_log, "Resetting device and capture for display %d", d->display);
	return open() && restartCapture();
}

int DDAGrabber::grabFrame(Image<ColorRgb>& image)
{
	if (!_isEnabled || _isDeviceInError)
	{
		if (_isDeviceInError)
		{
			Error(_log, "Cannot grab frame, device is in error state");
		}
		return -1;
	}

	if (!d->desktopDuplication && !resetDeviceAndCapture())
	{
		Error(_log, "Failed to open or restart capture for display %d", d->display);
		return -1;
	}

	HRESULT hr{ S_OK };
	hr = d->desktopDuplication->ReleaseFrame();
	if (FAILED(hr) && hr != DXGI_ERROR_INVALID_CALL)
	{
		LOG_ERROR(hr, "Failed to release frame");
	}

	CComPtr<IDXGIResource> desktopResource;
	DXGI_OUTDUPL_FRAME_INFO frameInfo = {};
	hr = d->desktopDuplication->AcquireNextFrame(500, &frameInfo, &desktopResource);

	if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL)
	{
		Debug(_log, "Access lost (hr=0x%08x), resetting capture.", hr);
		if (!restartCapture())
		{
			Error(_log, "Access lost - Failed to restart capture.");
		}
		return -1;
	}
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		return 0;
	}
	RETURN_IF_ERROR(hr, "Failed to acquire next frame", -1);


	CComPtr<ID3D11Texture2D> sourceTexture;
	hr = desktopResource->QueryInterface(&sourceTexture);
	RETURN_IF_ERROR(hr, "Failed to get 2D texture from resource", -1);

	//qDebug() << "DDAGrabber::grabFrame: _width: " << _width << ", height: " << _height	<< ", _pixelDecimation: " << _pixelDecimation;

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
	RETURN_IF_ERROR(hr, "D2D DrawBitmap failed", -1);

	// Copy the D2D result to the staging texture
	d->deviceContext->CopyResource(d->intermediateTexture, d->d2dConvertedTexture);

	// Map and copy to user image
	D3D11_MAPPED_SUBRESOURCE mapped;
	hr = d->deviceContext->Map(d->intermediateTexture, 0, D3D11_MAP_READ, 0, &mapped);
	RETURN_IF_ERROR(hr, "Failed to map final texture", -1);

	// CPU pixel copy from BGRA to output RGB image
	image.resize(_width, _height);
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

void DDAGrabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
	restartCapture();
}

bool DDAGrabber::setDisplayIndex(int index)
{
	bool rc = true;
	if (d->display != index)
	{
		d->display = index;
		rc = resetDeviceAndCapture();
	}
	return rc;
}

void DDAGrabber::setVideoMode(VideoMode mode)
{
	Grabber::setVideoMode(mode);
	restartCapture();
}

bool DDAGrabber::setPixelDecimation(int pixelDecimation)
{
	qDebug() << "Setting pixel decimation to" << pixelDecimation << ", current is" << _pixelDecimation;
	if (Grabber::setPixelDecimation(pixelDecimation))
	{
		restartCapture();
		return true;
	}

	return false;
}

QJsonObject DDAGrabber::discover(const QJsonObject& params)
{
	QJsonObject inputsDiscovered;
	if (isAvailable(false) && open())
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
	DebugIf(verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;
}
