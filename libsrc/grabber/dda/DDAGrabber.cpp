
// Platform-specific only
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dxgi1_2.h>
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
	D3D11_TEXTURE2D_DESC intermediateTextureDesc{};

	int display = 0;

	DXGI_MODE_ROTATION desktopRotation = DXGI_MODE_ROTATION_IDENTITY;
	int desktopWidth = 0;
	int desktopHeight = 0;

	int finalWidth = 0;
	int finalHeight = 0;
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

	static const D3D_DRIVER_TYPE driverTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};

	HRESULT hr{ S_OK };
	for (auto driverType : driverTypes)
	{
		hr = D3D11CreateDevice(
			nullptr,                 // Adapter to use
			driverType,              // Driver type (since we specified adapter)
			nullptr,                 // Software device (not used)
			0,                       // Flags
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

	hr = d->dxgiDevice->GetAdapter(&d->dxgiAdapter);
	RETURN_IF_ERROR(hr, "Failed to get DXGI adapter", false);

	return true;
}

bool DDAGrabber::restartCapture()
{
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

	d->desktopRotation = desc.Rotation;
	d->desktopWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
	d->desktopHeight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;

	_width = d->desktopWidth;
	_height = d->desktopHeight;

	if (_cropLeft + _cropRight >= _width || _cropTop + _cropBottom >= _height)
	{
		Error(_log, "Invalid cropping values which exceed the screen size. Cropping disabled.");
		_cropLeft = _cropRight = _cropTop = _cropBottom = 0;
	}

	int croppedWidth = _width - (_cropLeft + _cropRight);
	int croppedHeight = _height - (_cropTop + _cropBottom);
	int finalWidth = qMax(1, croppedWidth / _pixelDecimation);
	int finalHeight = qMax(1, croppedHeight / _pixelDecimation);

	Debug(_log, "Desktop size: %dx%d, cropping=%d,%d,%d,%d, decimation=%d, final image size=%dx%d",
		_width, _height, _cropLeft, _cropTop, _cropRight, _cropBottom, _pixelDecimation,
		finalWidth, finalHeight);

	if (finalWidth != d->finalWidth || finalHeight != d->finalHeight)
	{
		d->desktopDuplication.Release();
		Debug(_log, "New capture size detected. Creating Desktop Duplication for display %d", d->display);

		hr = d->dxgiOutput1->DuplicateOutput(d->device, &d->desktopDuplication);
		RETURN_IF_ERROR(hr, "Failed to create desktop duplication interface", false);

		d->finalWidth = finalWidth;
		d->finalHeight = finalHeight;
	}
	else
	{
		Debug(_log, "Reusing existing output for display %d", d->display);
	}

	return true;
}

int DDAGrabber::grabFrame(Image<ColorRgb>& image)
{
	if (!_isEnabled || _isDeviceInError)
	{
		if (_isDeviceInError)
		{
			Error(_log, "Cannot grab frame, device is in error state");
		}
		return 0;
	}

	if (!d->desktopDuplication && !resetDeviceAndCapture())
	{
		Error(_log, "Failed to open or restart capture for display %d", d->display);
		return -1;
	}

	if (d->desktopDuplication)
	{
		HRESULT hr = d->desktopDuplication->ReleaseFrame();
		if (FAILED(hr) && hr != DXGI_ERROR_INVALID_CALL)
		{
			LOG_ERROR(hr, "Failed to release frame");
		}
	}

	CComPtr<IDXGIResource> desktopResource;
	DXGI_OUTDUPL_FRAME_INFO frameInfo = {};
	HRESULT hr = d->desktopDuplication->AcquireNextFrame(500, &frameInfo, &desktopResource);
	if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL)
	{
		if (d->intermediateTexture)
		{
			d->deviceContext->Unmap(d->intermediateTexture, 0);
		}
		if (!restartCapture())
		{
			return -1;
		}
		return 0;
	}
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		qDebug() << "No new frame available, waiting for next frame";
		// Nothing changed on the screen in the last 500ms
		return 0;
	}
	RETURN_IF_ERROR(hr, "Failed to acquire next frame", -1);

	CComPtr<ID3D11Texture2D> texture;
	hr = desktopResource->QueryInterface(&texture);
	RETURN_IF_ERROR(hr, "Failed to get 2D texture", -1);

	D3D11_TEXTURE2D_DESC textureDesc;
	texture->GetDesc(&textureDesc);

	D3D11_BOX srcBox{};
	computeCropBox(textureDesc, srcBox);
	int cropW = srcBox.right - srcBox.left;
	int cropH = srcBox.bottom - srcBox.top;

	if (!d->intermediateTexture || d->intermediateTextureDesc.Width != cropW || d->intermediateTextureDesc.Height != cropH)
	{
		d->intermediateTexture.Release();
		d->intermediateTextureDesc = textureDesc;
		d->intermediateTextureDesc.Width = cropW;
		d->intermediateTextureDesc.Height = cropH;
		d->intermediateTextureDesc.Usage = D3D11_USAGE_STAGING;
		d->intermediateTextureDesc.BindFlags = 0;
		d->intermediateTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		d->intermediateTextureDesc.MiscFlags = 0;

		hr = d->device->CreateTexture2D(&d->intermediateTextureDesc, nullptr, &d->intermediateTexture);
		RETURN_IF_ERROR(hr, "Failed to create intermediate texture", -1);
	}

	if (_isCropping)
	{
		// Copy only the cropped region from GPU texture
		d->deviceContext->CopySubresourceRegion(
			d->intermediateTexture,    // Destination (CPU readable)
			0,                         // Destination subresource
			0,                         // dstX
			0,                         // dstY
			0,                         // dstZ
			texture,                   // Source texture (GPU-only)
			0,                         // Source subresource
			&srcBox                    // Cropped box (now rotated)
		);
	}
	else
	{
		// Copy the entire resource if no cropping is needed
		d->deviceContext->CopyResource(d->intermediateTexture, texture);
	}

	D3D11_MAPPED_SUBRESOURCE mapped = {};
	hr = d->deviceContext->Map(d->intermediateTexture, 0, D3D11_MAP_READ, 0, &mapped);
	RETURN_IF_ERROR(hr, "Failed to map texture", 0);

	if (textureDesc.Format != DXGI_FORMAT_B8G8R8A8_UNORM)
	{
		Error(_log, "Image captured is not in the expected DXGI_FORMAT_B8G8R8A8_UNORM format");
		d->deviceContext->Unmap(d->intermediateTexture, 0);
		return 0;
	}

	// The source dimensions for the pixel mapping are now the dimensions of the
	// intermediateTexture, which already reflect the cropping.
	const unsigned char* srcPixels = static_cast<const unsigned char*>(mapped.pData);
	const int srcWidth = d->intermediateTextureDesc.Width;
	const int srcHeight = d->intermediateTextureDesc.Height;
	const int srcPitch = mapped.RowPitch;

	image.resize(d->finalWidth, d->finalHeight);
	ColorRgb* rgbBuffer = image.memptr();

	if (!rgbBuffer)
	{
		d->deviceContext->Unmap(d->intermediateTexture, 0);
		return -1;
	}

	const int srcWidth1 = srcWidth - 1;
	const int srcHeight1 = srcHeight - 1;
	const int finalWidth1 = d->finalWidth - 1;
	const int finalHeight1 = d->finalHeight - 1;

	for (int y_dest = 0; y_dest < d->finalHeight; ++y_dest)
	{
		ColorRgb* destRowPtr = rgbBuffer + (y_dest * d->finalWidth);

		for (int x_dest = 0; x_dest < d->finalWidth; ++x_dest)
		{
			int src_x, src_y;

			switch (d->desktopRotation)
			{
			case DXGI_MODE_ROTATION_IDENTITY:
				src_x = x_dest * _pixelDecimation;
				src_y = y_dest * _pixelDecimation;
				break;
			case DXGI_MODE_ROTATION_ROTATE90:
				src_x = srcWidth1 - ((finalHeight1 - y_dest) * _pixelDecimation);
				src_y = (finalWidth1 - x_dest) * _pixelDecimation;
				break;
			case DXGI_MODE_ROTATION_ROTATE180:
				src_x = srcWidth1 - (x_dest * _pixelDecimation);
				src_y = srcHeight1 - (y_dest * _pixelDecimation);
				break;
			case DXGI_MODE_ROTATION_ROTATE270:
				src_x = (finalHeight1 - y_dest) * _pixelDecimation;
				src_y = srcHeight1 - ((finalWidth1 - x_dest) * _pixelDecimation);
				break;
			default:
				src_x = x_dest * _pixelDecimation;
				src_y = y_dest * _pixelDecimation;
				break;
			}

			src_x = std::clamp(src_x, 0, srcWidth1);
			src_y = std::clamp(src_y, 0, srcHeight1);

			const unsigned char* srcPixel = srcPixels + (src_y * srcPitch) + (src_x << 2);
			ColorRgb& destPixel = destRowPtr[x_dest];
			destPixel.red = srcPixel[2];
			destPixel.green = srcPixel[1];
			destPixel.blue = srcPixel[0];
		}
	}

	d->deviceContext->Unmap(d->intermediateTexture, 0);
	return 0;
}

bool DDAGrabber::resetDeviceAndCapture()
{
	qDebug() << "Resetting device and capture for display" << d->display;
	return open() && restartCapture();
}

void DDAGrabber::computeCropBox(const D3D11_TEXTURE2D_DESC& desc, D3D11_BOX& box) const
{
	switch (d->desktopRotation) {
	case DXGI_MODE_ROTATION_ROTATE90:
		box.left = _cropTop;
		box.right = desc.Width - _cropBottom;
		box.top = _cropRight;
		box.bottom = desc.Height - _cropLeft;
		break;
	case DXGI_MODE_ROTATION_ROTATE270:
		box.left = _cropBottom;
		box.right = desc.Width - _cropTop;
		box.top = _cropLeft;
		box.bottom = desc.Height - _cropRight;
		break;
	case DXGI_MODE_ROTATION_ROTATE180:
		box.left = _cropRight;
		box.right = desc.Width - _cropLeft;
		box.top = _cropBottom;
		box.bottom = desc.Height - _cropTop;
		break;
	case DXGI_MODE_ROTATION_IDENTITY:
	default:
		box.left = _cropLeft;
		box.right = desc.Width - _cropRight;
		box.top = _cropTop;
		box.bottom = desc.Height - _cropBottom;
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
		rc = restartCapture();
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
