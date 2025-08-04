
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
		hr = D3D11CreateDevice(nullptr, driverType, nullptr, 0, nullptr, 0,
			D3D11_SDK_VERSION, &d->device, nullptr, &d->deviceContext);
		if (SUCCEEDED(hr)) break;
	}

	RETURN_IF_ERROR(hr, "CreateDevice failed",false);

	hr = d->device->QueryInterface(&d->dxgiDevice);
	qDebug() << "Result: " << hr;
	RETURN_IF_ERROR(hr, "Failed to get DXGI device", false);

	hr = d->dxgiDevice->GetAdapter(&d->dxgiAdapter);

	qDebug() << "Result: " << hr;
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
		};

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
	_height = d-> desktopHeight;

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
		_width, _height,
		_cropLeft, _cropTop, _cropRight, _cropBottom,
		_pixelDecimation,
		d->finalWidth, d->finalHeight);

	if (finalWidth != d->finalWidth || finalHeight != d->finalHeight)
	{
		d->desktopDuplication.Release();

		Debug(_log, "New capture width and/or height detected. Creating Desktop-Duplication for display %d", d->display);
		d->dxgiOutput1->DuplicateOutput(d->device, &d->desktopDuplication);
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
	//qDebug() << "Grabbing frame for display" << d->display;
	if (!_isEnabled || _isDeviceInError)
	{
		if (_isDeviceInError)
		{
			Error(_log, "Cannot grab frame, device is in error state");
		}
		else
		{
			Debug(_log, "Grabber is not enabled, skipping frame grab");
		}
		return 0;
	}

	if (!d->desktopDuplication && (!open() || !restartCapture()))
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
		qDebug() << "Access lost or invalid call, Error: " << hr << ", resetting capture for display" << d->display;
		d->deviceContext->Unmap(d->intermediateTexture, 0);
		if (!restartCapture())
		{
			qDebug() << "Access lost or invalid call - Failed to restart capture for display" << d->display;
			return -1;
		}
		return 0;
	}
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		qDebug() << "No new frame available, waiting for next frame";
		// Nothing changed on the screen in the 500ms we waited.
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

	if (!d->intermediateTexture ||
		d->intermediateTextureDesc.Width != cropW || d->intermediateTextureDesc.Height != cropH
	)
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

	// Copy the texture data based on whether cropping is active
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

	D3D11_MAPPED_SUBRESOURCE mapped;
	hr = d->deviceContext->Map(d->intermediateTexture, 0, D3D11_MAP_READ, 0, &mapped);
	RETURN_IF_ERROR(hr, "Failed to map texture", 0);

	RET_CHECK(textureDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM, 0);

	qDebug() << "Before Final";
	qDebug() << "Image dimensions (target):" << image.width() << image.height();
	qDebug() << "Crop settings (L/R/T/B):" << _cropLeft << _cropRight << _cropTop << _cropBottom;
	qDebug() << "Desktop dimensions:" << d->desktopWidth << "x" << d->desktopHeight;

	int downscaleFactor = _pixelDecimation;

	// The source dimensions for the pixel mapping are now the dimensions of the
	// intermediateTexture, which already reflect the cropping.
	const unsigned char* srcPixels = static_cast<const unsigned char*>(mapped.pData);
	const int srcWidth = d->intermediateTextureDesc.Width;
	const int srcHeight = d->intermediateTextureDesc.Height;
	const int srcPitch = mapped.RowPitch;

	qDebug() << "Intermediate texture dimensions (actual source for pixel read):" << srcWidth << "x" << srcHeight;

	int finalWidth;
	int finalHeight;

	//// Calculate final dimensions based on rotation and downscale factor
	//// These calculations now apply to the `srcWidth` and `srcHeight` of the
	//// *cropped* intermediate texture.
	//switch (d->desktopRotation) {
	//case DXGI_MODE_ROTATION_ROTATE90:
	//case DXGI_MODE_ROTATION_ROTATE270:
	//	finalWidth = srcHeight / downscaleFactor;
	//	finalHeight = srcWidth / downscaleFactor;
	//	break;
	//case DXGI_MODE_ROTATION_ROTATE180:
	//case DXGI_MODE_ROTATION_IDENTITY:
	//default:
	//	finalWidth = srcWidth / downscaleFactor;
	//	finalHeight = srcHeight / downscaleFactor;
	//	break;
	//}

	//if (finalWidth == 0) finalWidth = 1;
	//if (finalHeight == 0) finalHeight = 1;

	finalWidth = d->finalWidth;
	finalHeight = d->finalHeight;

	image.resize(finalWidth, finalHeight);

	qDebug() << "After Final";
	qDebug() << "finalWidth: " << finalWidth << ", finalHeight: " << finalHeight;
	qDebug() << "d->finalWidth: " << d->finalWidth << ", d->finalHeight: " << d->finalHeight;
	qDebug() << "Image dimensions (target):" << image.width() << image.height();
	qDebug() << "_width: " << _width << ", _height: " << _height;
	qDebug() << "Crop settings (L/R/T/B):" << _cropLeft << _cropRight << _cropTop << _cropBottom;
	qDebug() << "Desktop dimensions:" << d->desktopWidth << "x" << d->desktopHeight;

	ColorRgb* rgbBuffer = image.memptr();

	if (!rgbBuffer) {
		d->deviceContext->Unmap(d->intermediateTexture, 0);
		return -1;
	}

	const int srcWidth_minus_1 = srcWidth - 1;
	const int srcHeight_minus_1 = srcHeight - 1;
	const int finalWidth_minus_1 = finalWidth - 1;
	const int finalHeight_minus_1 = finalHeight - 1;

	for (int y_dest = 0; y_dest < finalHeight; ++y_dest) {
		ColorRgb* destRowPtr = rgbBuffer + (y_dest * finalWidth);

		for (int x_dest = 0; x_dest < finalWidth; ++x_dest) {
			int src_x_original, src_y_original;

			// Apply rotation logic considering final output orientation
			switch (d->desktopRotation) {
			case DXGI_MODE_ROTATION_IDENTITY:
				src_x_original = x_dest * downscaleFactor;
				src_y_original = y_dest * downscaleFactor;
				break;
			case DXGI_MODE_ROTATION_ROTATE90:
				src_x_original = srcWidth_minus_1 - ((finalHeight_minus_1 - y_dest) * downscaleFactor);
				src_y_original = (finalWidth_minus_1 - x_dest) * downscaleFactor;
				break;
			case DXGI_MODE_ROTATION_ROTATE180:
				src_x_original = srcWidth_minus_1 - (x_dest * downscaleFactor);
				src_y_original = srcHeight_minus_1 - (y_dest * downscaleFactor);
				break;
			case DXGI_MODE_ROTATION_ROTATE270:
				src_x_original = (finalHeight_minus_1 - y_dest) * downscaleFactor;
				src_y_original = srcHeight_minus_1 - ((finalWidth_minus_1 - x_dest) * downscaleFactor);
				break;
			default:
				src_x_original = x_dest * downscaleFactor;
				src_y_original = y_dest * downscaleFactor;
				break;
			}

			// Ensure clamping is applied after all coordinate transformations
			src_x_original = (std::max)(0, (std::min)(src_x_original, srcWidth_minus_1));
			src_y_original = (std::max)(0, (std::min)(src_y_original, srcHeight_minus_1));

			const unsigned char* srcPixelPtr = srcPixels + (src_y_original * srcPitch) + (src_x_original << 2);

			ColorRgb* destPixelPtr = destRowPtr + x_dest;
			destPixelPtr->red = srcPixelPtr[2];
			destPixelPtr->green = srcPixelPtr[1];
			destPixelPtr->blue = srcPixelPtr[0];
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

void DDAGrabber::copyMappedToImage(const D3D11_MAPPED_SUBRESOURCE& mapped, Image<ColorRgb>& image) const
{
	const int width = d->finalWidth;
	const int height = d->finalHeight;

	image.resize(width, height);
	ColorRgb* dst = image.memptr();

	const unsigned char* src = static_cast<const unsigned char*>(mapped.pData);
	const int pitch = mapped.RowPitch;
	const int dec = _pixelDecimation;

	const int srcW = d->intermediateTextureDesc.Width;
	const int srcH = d->intermediateTextureDesc.Height;

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			int srcX = 0, srcY = 0;
			switch (d->desktopRotation)
			{
			case DXGI_MODE_ROTATION_IDENTITY:
				srcX = x * dec;
				srcY = y * dec;
				break;
			case DXGI_MODE_ROTATION_ROTATE90:
				srcX = srcH - (y * dec) - 1;
				srcY = x * dec;
				break;
			case DXGI_MODE_ROTATION_ROTATE180:
				srcX = srcW - (x * dec) - 1;
				srcY = srcH - (y * dec) - 1;
				break;
			case DXGI_MODE_ROTATION_ROTATE270:
				srcX = y * dec;
				srcY = srcW - (x * dec) - 1;
				break;
			}
			const unsigned char* px = src + srcY * pitch + srcX * 4;
			dst[y * width + x] = ColorRgb(px[2], px[1], px[0]);
		}
	}
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


//
//
//#include "grabber/dda/DDAGrabber.h"
//
//#define NOMINMAX // Prevents min/max macros from being defined by Windows.h
//
//#include <atlbase.h>
//#include <d3d11.h>
//#include <dxgi1_2.h>
//#include <d3dcommon.h>
//#include <physicalmonitorenumerationapi.h>
//#include <windows.h>
////#include <utility>
//
////#include <algorithm>
//
//#pragma comment(lib, "d3d9.lib")
//#pragma comment(lib, "dxva2.lib")
//
//#include <utils/Logger.h>
//
//
//namespace
//{
//// Driver types supported.
//constexpr D3D_DRIVER_TYPE kDriverTypes[] = {
//    D3D_DRIVER_TYPE_HARDWARE,
//    D3D_DRIVER_TYPE_WARP,
//    D3D_DRIVER_TYPE_REFERENCE,
//};
//
////// Feature levels supported.
////D3D_FEATURE_LEVEL kFeatureLevels[] = {
////	D3D_FEATURE_LEVEL_12_1,
////	D3D_FEATURE_LEVEL_12_0,
////	D3D_FEATURE_LEVEL_11_1,
////	D3D_FEATURE_LEVEL_11_0,
////	D3D_FEATURE_LEVEL_10_1,
////	D3D_FEATURE_LEVEL_10_0,
////	D3D_FEATURE_LEVEL_9_3,
////	D3D_FEATURE_LEVEL_9_2,
////	D3D_FEATURE_LEVEL_9_1
////};
//
//// Returns true if the two texture descriptors are compatible for copying.
//bool areTextureDescriptionsCompatible(D3D11_TEXTURE2D_DESC a, D3D11_TEXTURE2D_DESC b)
//{
//	return a.Width == b.Width && a.Height == b.Height && a.MipLevels == b.MipLevels && a.ArraySize == b.ArraySize &&
//	       a.Format == b.Format;
//}
//
//} // namespace
//
//// Logs a message along with the hex error HRESULT.
//#define LOG_ERROR(hr, msg) Error(_log, msg ": 0x%x", hr)
//
//// Checks if the HRESULT is an error, and if so, logs it and returns from the
//// current function.
//#define RETURN_IF_ERROR(hr, msg, returnValue)                                                                          \
//	if (FAILED(hr))                                                                                                    \
//	{                                                                                                                  \
//		LOG_ERROR(hr, msg);                                                                                            \
//		return returnValue;                                                                                            \
//	}
//
//// Checks if the condition is false, and if so, logs an error and returns from
//// the current function.
//#define RET_CHECK(cond, returnValue)                                                                                   \
//	if (!(cond))                                                                                                       \
//	{                                                                                                                  \
//		Error(_log, "Assertion failed: " #cond);                                                                       \
//		return returnValue;                                                                                            \
//	}
//
//// Private implementation. These member variables are here and not in the .h
//// so we don't have to include <atlbase.h> in the header and pollute everything
//// else that includes it.
//class DDAGrabberImpl
//{
//public:
//	// Created in the constructor.
//	CComPtr<ID3D11Device> device;
//	CComPtr<ID3D11DeviceContext> deviceContext;
//	CComPtr<IDXGIDevice> dxgiDevice;
//	CComPtr<IDXGIAdapter> dxgiAdapter;
//	CComPtr<IDXGIOutput1> dxgiOutput1; // Store output1 here for easier re-selection
//
//	// Created in restartCapture - only valid while desktop capture is in
//	// progress.
//	CComPtr<ID3D11Texture2D> intermediateTexture;
//	D3D11_TEXTURE2D_DESC intermediateTextureDesc;
//	CComPtr<IDXGIOutputDuplication> desktopDuplication;
//
//	int display = 0;
//	int desktopWidth = 0;
//	int desktopHeight = 0;
//	DXGI_MODE_ROTATION desktopRotation = DXGI_MODE_ROTATION_UNSPECIFIED;
//};
//
//DDAGrabber::DDAGrabber(int display, int cropLeft, int cropRight, int cropTop, int cropBottom)
//    : Grabber("GRABBER-DDA", cropLeft, cropRight, cropTop, cropBottom), d(new DDAGrabberImpl)
//{
//	_useImageResampler = false;
//	d->display = display;
//
//	//HRESULT hr = S_OK;
//
//	//// Iterate through driver types until we find one that succeeds.
//	//D3D_FEATURE_LEVEL featureLevel;
//	//for (D3D_DRIVER_TYPE driverType : kDriverTypes)
//	//{
//	//	hr = D3D11CreateDevice(nullptr, driverType, nullptr, 0, nullptr, 0,
//	//		D3D11_SDK_VERSION, &d->device, &featureLevel, &d->deviceContext);
//
//	//	//hr = D3D11CreateDevice(nullptr, driverType, nullptr, 0, kFeatureLevels, std::size(kFeatureLevels),
//	//	//                       D3D11_SDK_VERSION, &d->device, &featureLevel, &d->deviceContext);
//	//	if (SUCCEEDED(hr))
//	//	{
//	//		break;
//	//	}
//	//}
//	//RETURN_IF_ERROR(hr, "CreateDevice failed", );
//
//	//// Get the DXGI factory.
//	//hr = d->device.QueryInterface(&d->dxgiDevice);
//	//RETURN_IF_ERROR(hr, "Failed to get DXGI device", );
//
//	//// Get the factory's adapter.
//	//hr = d->dxgiDevice->GetAdapter(&d->dxgiAdapter);
//	//RETURN_IF_ERROR(hr, "Failed to get DXGI Adapter", );
//}
//
//DDAGrabber::~DDAGrabber()
//{
//}
//
//bool DDAGrabber::open()
//{
//	return true;
//}
//
//bool DDAGrabber::resetDeviceAndCapture()
//{
//	Info(_log, "Attempting full device and capture reset due to critical error.");
//
//	// Release all existing D3D resources
//	d->desktopDuplication.Release();
//	d->intermediateTexture.Release();
//	d->deviceContext.Release();
//	d->device.Release();
//	d->dxgiOutput1.Release(); // Release the output1 if stored
//	d->dxgiAdapter.Release();
//
//	HRESULT hr = S_OK;
//
//	// 1. Create DXGI Factory
//	CComPtr<IDXGIFactory1> dxgiFactory;
//	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&dxgiFactory);
//	RETURN_IF_ERROR(hr, "Failed to create DXGI Factory", false);
//
//	//// 2. Enumerate Adapters and select the one for the desired display
//	////    This assumes 'd->display' is still valid, or you have a way to re-select
//	////    the correct adapter/output.
//	////    For simplicity, let's just pick the first adapter for now. You might
//	////    need more sophisticated logic to pick the *correct* adapter.
//	//hr = dxgiFactory->EnumAdapters1(0, &d->dxgiAdapter); // Using adapter 0
//	//RETURN_IF_ERROR(hr, "Failed to enumerate DXGI adapter", false);
//
//	//// 3. Create D3D11 Device and Device Context
//	//D3D_FEATURE_LEVEL featureLevels[] = {
//	//	D3D_FEATURE_LEVEL_11_1,
//	//	D3D_FEATURE_LEVEL_11_0,
//	//	D3D_FEATURE_LEVEL_10_1,
//	//	D3D_FEATURE_LEVEL_10_0,
//	//};
//	//UINT numFeatureLevels = ARRAYSIZE(featureLevels);
//
//	//hr = D3D11CreateDevice(
//	//	nullptr,
//	//	//d->dxgiAdapter,         // Adapter to use
//	//	D3D_DRIVER_TYPE_UNKNOWN, // Driver type (since we specified adapter)
//	//	NULL,                    // Software device (not used)
//	//	0,                       // Flags
//	//	featureLevels,           // Feature levels to attempt
//	//	numFeatureLevels,        // Number of feature levels
//	//	D3D11_SDK_VERSION,       // SDK version
//	//	&d->device,              // Returned ID3D11Device
//	//	NULL,                    // Returned feature level
//	//	&d->deviceContext        // Returned ID3D11DeviceContext
//	//);
//	//RETURN_IF_ERROR(hr, "Failed to create D3D11 Device", false);
//
//	// Iterate through driver types until we find one that succeeds.
//	D3D_FEATURE_LEVEL featureLevel;
//	for (D3D_DRIVER_TYPE driverType : kDriverTypes)
//	{
//		//hr = D3D11CreateDevice(nullptr, driverType, nullptr, 0, nullptr, 0,
//		//	D3D11_SDK_VERSION, &d->device, &featureLevel, &d->deviceContext);
//		hr = D3D11CreateDevice(
//			nullptr,                 // Adapter to use
//			driverType,              // Driver type (since we specified adapter)
//			NULL,                    // Software device (not used)
//			0,                       // Flags
//			nullptr,                 // Feature levels to attempt
//			0,                       // Number of feature levels
//			D3D11_SDK_VERSION,       // SDK version
//			&d->device,              // Returned ID3D11Device
//			&featureLevel,           // Returned feature level
//			&d->deviceContext        // Returned ID3D11DeviceContext
//		);
//
//		//hr = D3D11CreateDevice(nullptr, driverType, nullptr, 0, kFeatureLevels, std::size(kFeatureLevels),
//		//                       D3D11_SDK_VERSION, &d->device, &featureLevel, &d->deviceContext);
//		if (SUCCEEDED(hr))
//		{
//			break;
//		}
//	}
//	RETURN_IF_ERROR(hr, "Failed to create D3D11 Device", false);
//
//	// Get the DXGI factory.
//	hr = d->device.QueryInterface(&d->dxgiDevice);
//	RETURN_IF_ERROR(hr, "Failed to get DXGI device", false);
//
//	// Get the factory's adapter.
//	hr = d->dxgiDevice->GetAdapter(&d->dxgiAdapter);
//
//	if (SUCCEEDED(hr))
//	{
//		qDebug() << "GetAdapter - Success!";
//	}
//	else
//	{
//		qDebug() << "GetAdapter - failed!";
//	}
//	RETURN_IF_ERROR(hr, "Failed to get DXGI Adapter", false);
//
//	// 4. Get the output that was selected.
//	CComPtr<IDXGIOutput> output;
//	hr = d->dxgiAdapter->EnumOutputs(d->display, &output);
//	RETURN_IF_ERROR(hr, "Failed to get output", false);
//
//
//	// 5. Get the descriptor which has the size of the display.
//	DXGI_OUTPUT_DESC desc;
//	hr = output->GetDesc(&desc);
//	RETURN_IF_ERROR(hr, "Failed to get output description", false);
//
//	d->desktopWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
//	d->desktopHeight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
//	d->desktopRotation = desc.Rotation;
//
//	// Recalculate _width and _height as they depend on desktopWidth/Height
//	_width = (d->desktopWidth - _cropLeft - _cropRight) / _pixelDecimation;
//	_height = (d->desktopHeight - _cropTop - _cropBottom) / _pixelDecimation;
//	if (_width <= 0) _width = 1; // Prevent zero dimensions
//	if (_height <= 0) _height = 1;
//
//	Info(_log, "Desktop size: %dx%d, cropping=%d,%d,%d,%d, decimation=%d, final image size=%dx%d",
//		d->desktopWidth,
//		d->desktopHeight,
//		_cropLeft, _cropTop, _cropRight, _cropBottom, _pixelDecimation, _width, _height);
//
//	// 6. Get the DXGIOutput1 interface.
//	hr = output.QueryInterface(&d->dxgiOutput1); // Store it directly in d->dxgiOutput1
//	RETURN_IF_ERROR(hr, "Failed to get output1", false);
//
//	// 7. Create the desktop duplication interface.
//	hr = d->dxgiOutput1->DuplicateOutput(d->device, &d->desktopDuplication);
//	RETURN_IF_ERROR(hr, "Failed to create desktop duplication interface", false);
//
//	return true;
//}
//
//
//// Modify restartCapture to call resetDeviceAndCapture when appropriate
//bool DDAGrabber::restartCapture()
//{
//	// If the device or adapter is already null, or a critical error occurred,
//	// we need a full reset.
//	if (!d->dxgiAdapter || !d->device || !d->desktopDuplication)
//	{
//		return resetDeviceAndCapture();
//	}
//
//	// Otherwise, attempt a "soft" restart (release duplication, then re-create)
//	HRESULT hr = S_OK;
//	d->desktopDuplication.Release();
//
//	// Get the output that was selected.
//	// We should reuse d->dxgiOutput1 if it's still valid, otherwise re-acquire.
//	CComPtr<IDXGIOutput> output; // Temp variable
//	if (d->dxgiOutput1) // If we already have the output1 interface
//	{
//		output = d->dxgiOutput1; // Use it directly
//	}
//	else // Otherwise, enumerate again (less efficient but safer if dxgiOutput1 got released)
//	{
//		hr = d->dxgiAdapter->EnumOutputs(d->display, &output);
//		RETURN_IF_ERROR(hr, "Failed to get output during soft restart", false);
//		hr = output.QueryInterface(&d->dxgiOutput1);
//		RETURN_IF_ERROR(hr, "Failed to get output1 during soft restart", false);
//	}
//
//	// Get the descriptor which has the size of the display.
//	DXGI_OUTPUT_DESC desc;
//	hr = output->GetDesc(&desc);
//	RETURN_IF_ERROR(hr, "Failed to get output description during soft restart", false);
//
//	d->desktopWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
//	d->desktopHeight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
//	d->desktopRotation = desc.Rotation;
//
//	_width = d->desktopWidth;
//	_height = d->desktopHeight;
//
//	if (_isCropping || _pixelDecimation > 1)
//	{
//		int width = (d->desktopWidth - _cropLeft - _cropRight) / _pixelDecimation;
//		int height = (d->desktopHeight - _cropTop - _cropBottom) / _pixelDecimation;
//
//		if (width < 1 || height < 1)
//		{
//			Error(_log, "Cropping and/or pixel decimation result in invalid output dimensions.");
//			return false;
//		}
//
//		_width = width;
//		_height = height;
//	}
//
//	Info(_log, "Desktop size: %dx%d, cropping=%d,%d,%d,%d, decimation=%d, final image size=%dx%d",
//		d->desktopWidth,
//		d->desktopHeight,
//		_cropLeft, _cropTop, _cropRight, _cropBottom, _pixelDecimation, _width, _height);
//
//	qDebug() << "image.width():" << _width
//		<< "image.height():" << _height
//		<< "cropLeft:" << _cropLeft
//		<< "cropTop:" << _cropTop
//		<< "cropRight:" << _cropRight
//		<< "cropBottom:" << _cropBottom;
//
//	// Create the desktop duplication interface.
//	hr = d->dxgiOutput1->DuplicateOutput(d->device, &d->desktopDuplication);
//	// If this fails with device removed, then the soft restart wasn't enough,
//	// and we need a full reset.
//	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL)
//	{
//		Info(_log, "Soft restart failed with 0x%lx, attempting full device reset.", hr);
//		return resetDeviceAndCapture();
//	}
//	RETURN_IF_ERROR(hr, "Failed to create desktop duplication interface", false);
//
//	return true;
//}
//
////bool DDAGrabber::restartCapture()
////{
////	if (!d->dxgiAdapter)
////	{
////		return false;
////	}
////
////	HRESULT hr = S_OK;
////
////	d->desktopDuplication.Release();
////
////	// Get the output that was selected.
////	CComPtr<IDXGIOutput> output;
////	hr = d->dxgiAdapter->EnumOutputs(d->display, &output);
////	RETURN_IF_ERROR(hr, "Failed to get output", false);
////
////	// Get the descriptor which has the size of the display.
////	DXGI_OUTPUT_DESC desc;
////	hr = output->GetDesc(&desc);
////	RETURN_IF_ERROR(hr, "Failed to get output description", false);
////
////	d->desktopWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
////	d->desktopHeight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
////	d->desktopRotation = desc.Rotation;
////
////	_width = d->desktopWidth;
////	_height = d->desktopHeight;
////
////	if (_isCropping || _pixelDecimation > 1)
////	{
////		int width = (d->desktopWidth - _cropLeft - _cropRight) / _pixelDecimation;
////		int height = (d->desktopHeight - _cropTop - _cropBottom) / _pixelDecimation;
////
////		if (width < 1 || height < 1)
////		{
////			Error(_log, "Cropping and/or pixel decimation result in invalid output dimensions.");
////			return false;
////		}
////
////		_width = width;
////		_height = height;
////	}
////
////
////	Info(_log, "Capture desktop: size: %dx%d, cropping=%d,%d,%d,%d, decimation=%d, final image size=%dx%d",
////			d->desktopWidth,
////			d->desktopHeight,
////			_cropLeft, _cropTop, _cropRight, _cropBottom, _pixelDecimation, _width, _height);
////
////	qDebug() << "image.width():" << _width
////			 << "image.height():" << _height
////			 << "cropLeft:" << _cropLeft
////			 << "cropTop:" << _cropTop
////			 << "cropRight:" << _cropRight
////			 << "cropBottom:" << _cropBottom
////			 << "_width:" << _width
////			 << "_height:" << _height;
////
////	// Get the DXGIOutput1 interface.
////	CComPtr<IDXGIOutput1> output1;
////	hr = output.QueryInterface(&output1);
////	RETURN_IF_ERROR(hr, "Failed to get output1", false);
////
////	// Create the desktop duplication interface.
////	hr = output1->DuplicateOutput(d->device, &d->desktopDuplication);
////
////	RETURN_IF_ERROR(hr, "Failed to create desktop duplication interface", false);
////
////	return true;
////}
//
//int DDAGrabber::grabFrame(Image<ColorRgb> &image)
//{
//	// Do nothing if the grabber is disabled.
//	if (!_isEnabled)
//	{
//		return 0;
//	}
//
//	// Start the capture if it's not already running.
//	if (!d->desktopDuplication && !restartCapture())
//	{
//		return -1;
//	}
//
//	HRESULT hr = S_OK;
//
//	// Release the last frame, if any.
//	hr = d->desktopDuplication->ReleaseFrame();
//	if (FAILED(hr))
//	{
//		if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL || hr == DXGI_ERROR_DEVICE_REMOVED)
//		{
//			Info(_log, "ReleaseFrame failed with 0x%lx, attempting full device reset.", hr);
//			if (!resetDeviceAndCapture()) // Call full reset
//			{
//				return -1;
//			}
//			return 0; // Return 0 to try again on next frame
//		}
//		LOG_ERROR(hr, "Failed to release frame");
//	}
//
//	// Acquire the next frame.
//	CComPtr<IDXGIResource> desktopResource;
//	DXGI_OUTDUPL_FRAME_INFO frameInfo;
//	hr = d->desktopDuplication->AcquireNextFrame(500, &frameInfo, &desktopResource);
//	if (FAILED(hr)) // Check FAILED macro for all failure codes
//	{
//		if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL || hr == DXGI_ERROR_DEVICE_REMOVED)
//		{
//			Info(_log, "AcquireNextFrame failed with 0x%lx, attempting full device reset.", hr);
//			if (!resetDeviceAndCapture()) // Call full reset
//			{
//				return -1;
//			}
//			return 0; // Return 0 to try again on next frame
//		}
//		else if (hr == DXGI_ERROR_WAIT_TIMEOUT)
//		{
//			// Nothing changed on the screen in the 500ms we waited.
//			return 0;
//		}
//		RETURN_IF_ERROR(hr, "Failed to acquire next frame", 0);
//	}
//
//	// Get the 2D texture.
//	CComPtr<ID3D11Texture2D> texture;
//	hr = desktopResource.QueryInterface(&texture);
//	RETURN_IF_ERROR(hr, "Failed to get 2D texture", 0);
//
//	// The texture we acquired is on the GPU and can't be accessed from the CPU,
//	// so we have to copy it into another texture that can.
//	D3D11_TEXTURE2D_DESC textureDesc;
//	texture->GetDesc(&textureDesc);
//
//	qDebug() << "image.width():" << _width
//		<< "image.height():" << _height
//		<< "cropLeft:" << _cropLeft
//		<< "cropTop:" << _cropTop
//		<< "cropRight:" << _cropRight
//		<< "cropBottom:" << _cropBottom
//		<< "_width:" << _width
//		<< "_height:" << _height;
//
//	// --- IMPORTANT DEBUGGING LINE ---
//	qDebug() << "Raw DDA Texture dimensions:" << textureDesc.Width << "x" << textureDesc.Height;
//	// --- END IMPORTANT DEBUGGING LINE ---
//
//	// Define the cropped source rectangle for CopySubresourceRegion
//	D3D11_BOX srcBox = {};
//	int effectiveSrcWidth = textureDesc.Width;
//	int effectiveSrcHeight = textureDesc.Height;
//
//	if (_isCropping)
//	{
//		switch (d->desktopRotation) {
//		case DXGI_MODE_ROTATION_ROTATE90:
//			srcBox.left = _cropTop;
//			srcBox.right = textureDesc.Width - _cropBottom;
//			srcBox.top = _cropRight;
//			srcBox.bottom = textureDesc.Height - _cropLeft;
//			break;
//		case DXGI_MODE_ROTATION_ROTATE270:
//			srcBox.left = _cropBottom;
//			srcBox.right = textureDesc.Width - _cropTop;
//			srcBox.top = _cropLeft;
//			srcBox.bottom = textureDesc.Height - _cropRight;
//			break;
//		case DXGI_MODE_ROTATION_ROTATE180:
//			srcBox.left = _cropRight;
//			srcBox.right = textureDesc.Width - _cropLeft;
//			srcBox.top = _cropBottom;
//			srcBox.bottom = textureDesc.Height - _cropTop;
//			break;
//		case DXGI_MODE_ROTATION_IDENTITY:
//		default:
//			srcBox.left = _cropLeft;
//			srcBox.right = textureDesc.Width - _cropRight;
//			srcBox.top = _cropTop;
//			srcBox.bottom = textureDesc.Height - _cropBottom;
//			break;
//		}
//
//		srcBox.front = 0;
//		srcBox.back = 1;
//
//		qDebug() << "Cropped source box:" << srcBox.left << srcBox.right << srcBox.top << srcBox.bottom;
//
//		effectiveSrcWidth = srcBox.right - srcBox.left;
//		effectiveSrcHeight = srcBox.bottom - srcBox.top;
//
//		qDebug() << "Cropped source dimensions:" << effectiveSrcWidth << "x" << effectiveSrcHeight;
//
//		// Ensure crop values don't result in negative or zero dimensions
//		if (effectiveSrcWidth <= 0 || effectiveSrcHeight <= 0)
//		{
//			Error(_log, "Cropping resulted in invalid dimensions (%dx%d) for UNROTATED texture. Skipping frame.", effectiveSrcWidth, effectiveSrcHeight);
//			return -1;
//		}
//
//		// Ensure crop values don't result in negative or zero dimensions
//		if (effectiveSrcWidth / _pixelDecimation < 1 || effectiveSrcHeight / _pixelDecimation < 1)
//		{
//			//image.resize(1, 1); // Resize to 1x1 to avoid issues with zero dimensions
//
//			Error(_log, "Cropping & pixel decimation resulted in invalid output image dimensions. Skipping frame.");
//			return -1;
//		}
//	}
//	// If not cropping, effectiveSrcWidth/Height remain textureDesc.Width/Height (no rotation applied to full frame)
//
//
//	// Create a new intermediate texture if we have not done already, or the
//	// existing one is incompatible with the required size (either cropped or full).
//	if (!d->intermediateTexture ||
//		d->intermediateTextureDesc.Width != effectiveSrcWidth ||
//		d->intermediateTextureDesc.Height != effectiveSrcHeight)
//	{
//		Info(_log, "Creating intermediate texture (cropping: %s) with dimensions %dx%d", _isCropping ? "true" : "false", effectiveSrcWidth, effectiveSrcHeight);
//
//        // With this line:
//
//        //DebugIf(true,_log, "Releasing previous intermediate texture if any");
//
//
//		d->intermediateTexture.Release();
//
//		d->intermediateTextureDesc = textureDesc; // Start with original desc to preserve format, mip, array size etc.
//		d->intermediateTextureDesc.Width = effectiveSrcWidth;
//		d->intermediateTextureDesc.Height = effectiveSrcHeight;
//		d->intermediateTextureDesc.Usage = D3D11_USAGE_STAGING;
//		d->intermediateTextureDesc.BindFlags = 0;
//		d->intermediateTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
//		d->intermediateTextureDesc.MiscFlags = 0;
//
//		hr = d->device->CreateTexture2D(&d->intermediateTextureDesc, nullptr, &d->intermediateTexture);
//		RETURN_IF_ERROR(hr, "Failed to create intermediate texture", 0);
//	}
//
//	// Copy the texture data based on whether cropping is active
//	if (_isCropping)
//	{
//		// Copy only the cropped region from GPU texture
//		d->deviceContext->CopySubresourceRegion(
//			d->intermediateTexture,    // Destination (CPU readable)
//			0,                         // Destination subresource
//			0,                         // dstX
//			0,                         // dstY
//			0,                         // dstZ
//			texture,                   // Source texture (GPU-only)
//			0,                         // Source subresource
//			&srcBox                    // Cropped box (now rotated)
//		);
//		RETURN_IF_ERROR(hr, "Failed to copy cropped region to intermediate texture", 0);
//	}
//	else
//	{
//		// Copy the entire resource if no cropping is needed
//		d->deviceContext->CopyResource(d->intermediateTexture, texture);
//		RETURN_IF_ERROR(hr, "Failed to copy resource to intermediate texture", 0);
//	}
//
//	// Map the texture so we can access its pixels.
//	D3D11_MAPPED_SUBRESOURCE resource;
//	hr = d->deviceContext->Map(d->intermediateTexture, 0, D3D11_MAP_READ, 0, &resource);
//	RETURN_IF_ERROR(hr, "Failed to map texture", 0);
//
//	// Copy the texture to the output image.
//	RET_CHECK(textureDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM, 0);
//
//	qDebug() << "Image dimensions (target):" << image.width() << image.height();
//	qDebug() << "Crop settings (L/R/T/B):" << _cropLeft << _cropRight << _cropTop << _cropBottom;
//	qDebug() << "Desktop dimensions:" << d->desktopWidth << "x" << d->desktopHeight;
//
//	int downscaleFactor = _pixelDecimation;
//
//	// The source dimensions for the pixel mapping are now the dimensions of the
//	// intermediateTexture, which already reflect the cropping.
//	const unsigned char* srcPixels = static_cast<const unsigned char*>(resource.pData);
//	const int srcWidth = d->intermediateTextureDesc.Width;
//	const int srcHeight = d->intermediateTextureDesc.Height;
//	const int srcPitch = resource.RowPitch;
//
//	qDebug() << "Intermediate texture dimensions (actual source for pixel read):" << srcWidth << "x" << srcHeight;
//
//	int finalWidth;
//	int finalHeight;
//
//	// Calculate final dimensions based on rotation and downscale factor
//	// These calculations now apply to the `srcWidth` and `srcHeight` of the
//	// *cropped* intermediate texture.
//	switch (d->desktopRotation) {
//	case DXGI_MODE_ROTATION_ROTATE90:
//	case DXGI_MODE_ROTATION_ROTATE270:
//		finalWidth = srcHeight / downscaleFactor;
//		finalHeight = srcWidth / downscaleFactor;
//		break;
//	case DXGI_MODE_ROTATION_ROTATE180:
//	case DXGI_MODE_ROTATION_IDENTITY:
//	default:
//		finalWidth = srcWidth / downscaleFactor;
//		finalHeight = srcHeight / downscaleFactor;
//		break;
//	}
//
//	if (finalWidth == 0) finalWidth = 1;
//	if (finalHeight == 0) finalHeight = 1;
//
//	ColorRgb* rgbBuffer = image.memptr();
//
//	if (!rgbBuffer) {
//		d->deviceContext->Unmap(d->intermediateTexture, 0);
//		return -1;
//	}
//
//	const int srcWidth_minus_1 = srcWidth - 1;
//	const int srcHeight_minus_1 = srcHeight - 1;
//	const int finalWidth_minus_1 = finalWidth - 1;
//	const int finalHeight_minus_1 = finalHeight - 1;
//
//	for (int y_dest = 0; y_dest < finalHeight; ++y_dest) {
//		ColorRgb* destRowPtr = rgbBuffer + (y_dest * finalWidth);
//
//		for (int x_dest = 0; x_dest < finalWidth; ++x_dest) {
//			int src_x_original, src_y_original;
//
//			// Apply rotation logic considering final output orientation
//			switch (d->desktopRotation) {
//			case DXGI_MODE_ROTATION_IDENTITY:
//				src_x_original = x_dest * downscaleFactor;
//				src_y_original = y_dest * downscaleFactor;
//				break;
//			case DXGI_MODE_ROTATION_ROTATE90:
//				src_x_original = srcWidth_minus_1 - ((finalHeight_minus_1 - y_dest) * downscaleFactor);
//				src_y_original = (finalWidth_minus_1 - x_dest) * downscaleFactor;
//				break;
//			case DXGI_MODE_ROTATION_ROTATE180:
//				src_x_original = srcWidth_minus_1 - (x_dest * downscaleFactor);
//				src_y_original = srcHeight_minus_1 - (y_dest * downscaleFactor);
//				break;
//			case DXGI_MODE_ROTATION_ROTATE270:
//				src_x_original = (finalHeight_minus_1 - y_dest) * downscaleFactor;
//				src_y_original = srcHeight_minus_1 - ((finalWidth_minus_1 - x_dest) * downscaleFactor);
//				break;
//			default:
//				src_x_original = x_dest * downscaleFactor;
//				src_y_original = y_dest * downscaleFactor;
//				break;
//			}
//
//			// Ensure clamping is applied after all coordinate transformations
//			src_x_original = (std::max)(0, (std::min)(src_x_original, srcWidth_minus_1));
//			src_y_original = (std::max)(0, (std::min)(src_y_original, srcHeight_minus_1));
//
//			const unsigned char* srcPixelPtr = srcPixels + (src_y_original * srcPitch) + (src_x_original << 2);
//
//			ColorRgb* destPixelPtr = destRowPtr + x_dest;
//			destPixelPtr->red = srcPixelPtr[2];
//			destPixelPtr->green = srcPixelPtr[1];
//			destPixelPtr->blue = srcPixelPtr[0];
//		}
//	}
//
//	// Unmap the texture
//	d->deviceContext->Unmap(d->intermediateTexture, 0);
//
//	return 0;
//}
//
//void DDAGrabber::setVideoMode(VideoMode mode)
//{
//	Grabber::setVideoMode(mode);
//	restartCapture();
//}
//
//bool DDAGrabber::setPixelDecimation(int pixelDecimation)
//{
//	if (Grabber::setPixelDecimation(pixelDecimation))
//		return restartCapture();
//
//	return false;
//}
//
