#include "grabber/dda/DDAGrabber.h"

#include <atlbase.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <physicalmonitorenumerationapi.h>
#include <windows.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dxva2.lib")

namespace
{
// Driver types supported.
constexpr D3D_DRIVER_TYPE kDriverTypes[] = {
    D3D_DRIVER_TYPE_HARDWARE,
    D3D_DRIVER_TYPE_WARP,
    D3D_DRIVER_TYPE_REFERENCE,
};

// Feature levels supported.
D3D_FEATURE_LEVEL kFeatureLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
                                      D3D_FEATURE_LEVEL_9_1};

// Returns true if the two texture descriptors are compatible for copying.
bool areTextureDescriptionsCompatible(D3D11_TEXTURE2D_DESC a, D3D11_TEXTURE2D_DESC b)
{
	return a.Width == b.Width && a.Height == b.Height && a.MipLevels == b.MipLevels && a.ArraySize == b.ArraySize &&
	       a.Format == b.Format;
}

} // namespace

// Logs a message along with the hex error HRESULT.
#define LOG_ERROR(hr, msg) Error(_log, msg ": 0x%x", hr)

// Checks if the HRESULT is an error, and if so, logs it and returns from the
// current function.
#define RETURN_IF_ERROR(hr, msg, returnValue)                                                                          \
	if (FAILED(hr))                                                                                                    \
	{                                                                                                                  \
		LOG_ERROR(hr, msg);                                                                                            \
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

// Private implementation. These member variables are here and not in the .h
// so we don't have to include <atlbase.h> in the header and pollute everything
// else that includes it.
class DDAGrabberImpl
{
public:
	int display = 0;
	int desktopWidth = 0;
	int desktopHeight = 0;

	// Created in the constructor.
	CComPtr<ID3D11Device> device;
	CComPtr<ID3D11DeviceContext> deviceContext;
	CComPtr<IDXGIDevice> dxgiDevice;
	CComPtr<IDXGIAdapter> dxgiAdapter;

	// Created in restartCapture - only valid while desktop capture is in
	// progress.
	CComPtr<IDXGIOutputDuplication> desktopDuplication;
	CComPtr<ID3D11Texture2D> intermediateTexture;
	D3D11_TEXTURE2D_DESC intermediateTextureDesc;
};

DDAGrabber::DDAGrabber(int display, int cropLeft, int cropRight, int cropTop, int cropBottom)
    : Grabber("GRABBER-DDA", cropLeft, cropRight, cropTop, cropBottom), d(new DDAGrabberImpl)
{
	d->display = display;

	HRESULT hr = S_OK;

	// Iterate through driver types until we find one that succeeds.
	D3D_FEATURE_LEVEL featureLevel;
	for (D3D_DRIVER_TYPE driverType : kDriverTypes)
	{
		hr = D3D11CreateDevice(nullptr, driverType, nullptr, 0, kFeatureLevels, std::size(kFeatureLevels),
		                       D3D11_SDK_VERSION, &d->device, &featureLevel, &d->deviceContext);
		if (SUCCEEDED(hr))
		{
			break;
		}
	}
	RETURN_IF_ERROR(hr, "CreateDevice failed", );

	// Get the DXGI factory.
	hr = d->device.QueryInterface(&d->dxgiDevice);
	RETURN_IF_ERROR(hr, "Failed to get DXGI device", );

	// Get the factory's adapter.
	hr = d->dxgiDevice->GetAdapter(&d->dxgiAdapter);
	RETURN_IF_ERROR(hr, "Failed to get DXGI Adapter", );
}

DDAGrabber::~DDAGrabber()
{
}

bool DDAGrabber::restartCapture()
{
	if (!d->dxgiAdapter)
	{
		return false;
	}

	HRESULT hr = S_OK;

	d->desktopDuplication.Release();

	// Get the output that was selected.
	CComPtr<IDXGIOutput> output;
	hr = d->dxgiAdapter->EnumOutputs(d->display, &output);
	RETURN_IF_ERROR(hr, "Failed to get output", false);

	// Get the descriptor which has the size of the display.
	DXGI_OUTPUT_DESC desc;
	hr = output->GetDesc(&desc);
	RETURN_IF_ERROR(hr, "Failed to get output description", false);

	d->desktopWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
	d->desktopHeight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
	_width = (d->desktopWidth - _cropLeft - _cropRight) / _pixelDecimation;
	_height = (d->desktopHeight - _cropTop - _cropBottom) / _pixelDecimation;
	Info(_log, "Desktop size: %dx%d, cropping=%d,%d,%d,%d, decimation=%d, final image size=%dx%d", d->desktopWidth,
	     d->desktopHeight, _cropLeft, _cropTop, _cropRight, _cropBottom, _pixelDecimation, _width, _height);

	// Get the DXGIOutput1 interface.
	CComPtr<IDXGIOutput1> output1;
	hr = output.QueryInterface(&output1);
	RETURN_IF_ERROR(hr, "Failed to get output1", false);

	// Create the desktop duplication interface.
	hr = output1->DuplicateOutput(d->device, &d->desktopDuplication);
	RETURN_IF_ERROR(hr, "Failed to create desktop duplication interface", false);

	return true;
}

int DDAGrabber::grabFrame(Image<ColorRgb> &image)
{
	// Do nothing if we're disabled.
	if (!_isEnabled)
	{
		return 0;
	}

	// Start the capture if it's not already running.
	if (!d->desktopDuplication)
	{
		if (!restartCapture())
		{
			setEnabled(false);
			return -1;
		}
	}

	HRESULT hr = S_OK;

	// Release the last frame, if any.
	hr = d->desktopDuplication->ReleaseFrame();
	if (FAILED(hr) && hr != DXGI_ERROR_INVALID_CALL)
	{
		LOG_ERROR(hr, "Failed to release frame");
	}

	// Acquire the next frame.
	CComPtr<IDXGIResource> desktopResource;
	DXGI_OUTDUPL_FRAME_INFO frameInfo;
	hr = d->desktopDuplication->AcquireNextFrame(INFINITE, &frameInfo, &desktopResource);
	if (hr == DXGI_ERROR_ACCESS_LOST)
	{
		if (!restartCapture())
		{
			setEnabled(false);
			return -1;
		}
		return 0;
	}
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		// This shouldn't happen since we specified an INFINITE timeout.
		return 0;
	}
	RETURN_IF_ERROR(hr, "Failed to acquire next frame", 0);

	// Get the 2D texture.
	CComPtr<ID3D11Texture2D> texture;
	hr = desktopResource.QueryInterface(&texture);
	RETURN_IF_ERROR(hr, "Failed to get 2D texture", 0);

	// The texture we acquired is on the GPU and can't be accessed from the CPU,
	// so we have to copy it into another texture that can.
	D3D11_TEXTURE2D_DESC textureDesc;
	texture->GetDesc(&textureDesc);

	// Create a new intermediate texture if we haven't done so already, or the
	// existing one is incompatible with the acquired texture (i.e. it has
	// different dimensions).
	if (!d->intermediateTexture || !areTextureDescriptionsCompatible(d->intermediateTextureDesc, textureDesc))
	{
		Info(_log, "Creating intermediate texture");
		d->intermediateTexture.Release();

		d->intermediateTextureDesc = textureDesc;
		d->intermediateTextureDesc.Usage = D3D11_USAGE_STAGING;
		d->intermediateTextureDesc.BindFlags = 0;
		d->intermediateTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		d->intermediateTextureDesc.MiscFlags = 0;

		hr = d->device->CreateTexture2D(&d->intermediateTextureDesc, nullptr, &d->intermediateTexture);
		RETURN_IF_ERROR(hr, "Failed to create intermediate texture", 0);
	}

	// Copy the texture to the intermediate texture.
	d->deviceContext->CopyResource(d->intermediateTexture, texture);
	RETURN_IF_ERROR(hr, "Failed to copy texture", 0);

	// Map the texture so we can access its pixels.
	D3D11_MAPPED_SUBRESOURCE resource;
	hr = d->deviceContext->Map(d->intermediateTexture, 0, D3D11_MAP_READ, 0, &resource);
	RETURN_IF_ERROR(hr, "Failed to map texture", 0);

	// Copy the texture to the output image.
	RET_CHECK(textureDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM, 0);

	ColorRgb *dest = image.memptr();
	for (size_t destY = 0, srcY = _cropTop; destY < image.height(); destY++, srcY += _pixelDecimation)
	{
		uint32_t *src =
		    reinterpret_cast<uint32_t *>(reinterpret_cast<unsigned char *>(resource.pData) + srcY * resource.RowPitch) +
		    _cropLeft;
		for (size_t destX = 0; destX < image.width(); destX++, src += _pixelDecimation, dest++)
		{
			*dest = ColorRgb{static_cast<uint8_t>(((*src) >> 16) & 0xff), static_cast<uint8_t>(((*src) >> 8) & 0xff),
			                 static_cast<uint8_t>(((*src) >> 0) & 0xff)};
		}
	}

	return 0;
}

void DDAGrabber::setVideoMode(VideoMode mode)
{
	Grabber::setVideoMode(mode);
	restartCapture();
}

bool DDAGrabber::setPixelDecimation(int pixelDecimation)
{
	if (Grabber::setPixelDecimation(pixelDecimation))
		return restartCapture();

	return false;
}

void DDAGrabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	// Grabber::setCropping rejects the cropped size if it is larger than _width
	// and _height, so temporarily set those back to the original pre-cropped full
	// desktop sizes first. They'll be set back to the cropped sizes by
	// restartCapture.
	_width = d->desktopWidth;
	_height = d->desktopHeight;
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

QJsonObject DDAGrabber::discover(const QJsonObject &params)
{
	QJsonObject ret;
	if (!d->dxgiAdapter)
	{
		return ret;
	}

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

	ret["video_inputs"] = videoInputs;
	if (!videoInputs.isEmpty())
	{
		ret["device"] = "dda";
		ret["device_name"] = "DXGI DDA";
		ret["type"] = "screen";
		ret["default"] = QJsonObject{
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

	return ret;
}
