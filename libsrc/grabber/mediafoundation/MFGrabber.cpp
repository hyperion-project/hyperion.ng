#include "MFSourceReaderCB.h"
#include "grabber/MFGrabber.h"

// Constants
namespace { const bool verbose = false; }

MFGrabber::MFGrabber(const QString & device, unsigned width, unsigned height, unsigned fps, int pixelDecimation, QString flipMode)
	: Grabber("V4L2:"+device)
	, _currentDeviceName(device)
	, _newDeviceName(device)
	, _hr(S_FALSE)
	, _sourceReader(nullptr)
	, _pixelDecimation(pixelDecimation)
	, _lineLength(-1)
	, _frameByteSize(-1)
	, _noSignalCounterThreshold(40)
	, _noSignalCounter(0)
	, _fpsSoftwareDecimation(1)
	, _brightness(0)
	, _contrast(0)
	, _saturation(0)
	, _hue(0)
	, _currentFrame(0)
	, _noSignalThresholdColor(ColorRgb{0,0,0})
	, _signalDetectionEnabled(true)
	, _cecDetectionEnabled(true)
	, _noSignalDetected(false)
	, _initialized(false)
	, _x_frac_min(0.25)
	, _y_frac_min(0.25)
	, _x_frac_max(0.75)
	, _y_frac_max(0.75)
{
	setWidthHeight(width, height);
	setFramerate(fps);
	setFlipMode(flipMode);

	CoInitializeEx(0, COINIT_MULTITHREADED);
	_hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	if(FAILED(_hr))
		CoUninitialize();
	else
		_sourceReaderCB = new SourceReaderCB(this);
}

MFGrabber::~MFGrabber()
{
	uninit();

	SAFE_RELEASE(_sourceReader);
	SAFE_RELEASE(_sourceReaderCB);

	if(SUCCEEDED(_hr) && SUCCEEDED(MFShutdown()))
		CoUninitialize();
}

bool MFGrabber::init()
{
	if(!_initialized && SUCCEEDED(_hr))
	{
		QString foundDevice = "";
		int     foundIndex = -1, bestGuess = -1, bestGuessMinX = INT_MAX, bestGuessMinFPS = INT_MAX;
		bool    autoDiscovery = (QString::compare(_currentDeviceName, "auto", Qt::CaseInsensitive) == 0 );

		// enumerate the video capture devices on the user's system
		enumVideoCaptureDevices();

		if(!autoDiscovery && !_deviceProperties.contains(_currentDeviceName))
		{
			Debug(_log, "Device '%s' is not available. Changing to auto.", QSTRING_CSTR(_currentDeviceName));
			autoDiscovery = true;
		}

		if(autoDiscovery)
		{
			Debug(_log, "Forcing auto discovery device");
			if(_deviceProperties.count()>0)
			{
				foundDevice = _deviceProperties.firstKey();
				_currentDeviceName = foundDevice;
				Debug(_log, "Auto discovery set to %s", QSTRING_CSTR(_currentDeviceName));
			}
		}
		else
			foundDevice = _currentDeviceName;

		if(foundDevice.isNull() || foundDevice.isEmpty() || !_deviceProperties.contains(foundDevice))
		{
			Error(_log, "Could not find any capture device");
			return false;
		}

		MFGrabber::DeviceProperties dev = _deviceProperties[foundDevice];

		Debug(_log,  "Searching for %s %d x %d @ %d fps (%s)", QSTRING_CSTR(foundDevice), _width, _height,_fps, QSTRING_CSTR(pixelFormatToString(_pixelFormat)));

		for( int i = 0; i < dev.valid.count() && foundIndex < 0; ++i )
		{
			bool strict = false;
			const auto& val = dev.valid[i];

			if(bestGuess == -1 || (val.x <= bestGuessMinX && val.x >= 640 && val.fps <= bestGuessMinFPS && val.fps >= 10))
			{
				bestGuess = i;
				bestGuessMinFPS = val.fps;
				bestGuessMinX = val.x;
			}

			if(_width && _height)
			{
				strict = true;
				if(val.x != _width || val.y != _height)
					continue;
			}

			if(_fps && _fps!=15)
			{
				strict = true;
				if(val.fps != _fps)
					continue;
			}

			if(_pixelFormat != PixelFormat::NO_CHANGE)
			{
				strict = true;
				if(val.pf != _pixelFormat)
					continue;
			}

			if(strict && (val.fps <= 60 || _fps != 15))
				foundIndex = i;
		}

		if(foundIndex < 0 && bestGuess >= 0)
		{
			if(!autoDiscovery && _width != 0 && _height != 0)
				Warning(_log, "Selected resolution not found in supported modes. Forcing best resolution");
			else
				Debug(_log, "Forcing best resolution");

			foundIndex = bestGuess;
		}

		if(foundIndex>=0)
		{
			if(SUCCEEDED(init_device(foundDevice, dev.valid[foundIndex])))
				_initialized = true;
		}
		else
			Error(_log, "Could not find any capture device settings");
	}

	return _initialized;
}

void MFGrabber::uninit()
{
	// stop if the grabber was not stopped
	if(_initialized)
	{
		Debug(_log,"uninit grabber: %s", QSTRING_CSTR(_newDeviceName));
		stop();
	}
}

HRESULT MFGrabber::init_device(QString deviceName, DevicePropertiesItem props)
{
	PixelFormat pixelformat = GetPixelFormatForGuid(props.guid);
	QString error, guid = _deviceProperties[deviceName].name;
	int size = guid.length() + 1024;
	wchar_t *name = new wchar_t[size];
	memset(name, 0, size);
	guid.toWCharArray(name);
	IMFMediaSource* device = nullptr;
	IMFAttributes* deviceAttributes = nullptr, *sourceReaderAttributes = nullptr;
	IAMVideoProcAmp *pProcAmp = nullptr;
	IMFMediaType* type = nullptr;
	HRESULT hr = S_OK;

	Debug(_log, "Init %s, %d x %d @ %d fps (%s)", QSTRING_CSTR(deviceName), props.x, props.y, props.fps, QSTRING_CSTR(pixelFormatToString(pixelformat)));
	DebugIf(verbose, _log, "Symbolic link: %s", QSTRING_CSTR(guid));

	hr = MFCreateAttributes(&deviceAttributes, 2);
	if(FAILED(hr))
	{
		error = QString("Could not create device attributes (%1)").arg(hr);
		goto done;
	}

	hr = deviceAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if(FAILED(hr))
	{
		error = QString("SetGUID_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE (%1)").arg(hr);
		goto done;
	}

	if(FAILED(deviceAttributes->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, (LPCWSTR)name)) && _sourceReaderCB)
	{
		error = QString("IMFAttributes_SetString_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK (%1)").arg(hr);
		goto done;
	}

	hr = MFCreateDeviceSource(deviceAttributes, &device);
	if(FAILED(hr))
	{
		error = QString("MFCreateDeviceSource (%1)").arg(hr);
		goto done;
	}

	if(!device)
	{
		error = QString("Could not open device (%1)").arg(hr);
		goto done;
	}
	else
		Debug(_log, "Device opened");

	// Set Brightness/Contrast/Saturation/Hue
	if (_brightness != 0 || _contrast != 0 || _saturation != 0 || _hue != 0)
	{
		if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&pProcAmp))))
		{
			long lMin, lMax, lStep, lDefault, lCaps, Val;
			if (_brightness != 0)
			{
				if (SUCCEEDED(pProcAmp->GetRange(VideoProcAmp_Brightness, &lMin, &lMax, &lStep, &lDefault, &lCaps)))
				{
					Debug(_log, "Brightness: min=%i, max=%i, default=%i", lMin, lMax, lDefault);

					if (SUCCEEDED(pProcAmp->Get(VideoProcAmp_Brightness, &Val,  &lCaps)))
						Debug(_log, "Current brightness set to: %i",Val);

					if (SUCCEEDED(pProcAmp->Set(VideoProcAmp_Brightness, _brightness, VideoProcAmp_Flags_Manual)))
						Debug(_log, "Brightness set to: %i",_brightness);
					else
						Error(_log, "Could not set brightness");
				}
				else
					Error(_log, "Brightness is not supported by the grabber");
			}

			if (_contrast != 0)
			{
				if (SUCCEEDED(pProcAmp->GetRange(VideoProcAmp_Contrast, &lMin, &lMax, &lStep, &lDefault, &lCaps)))
				{
					Debug(_log, "Contrast: min=%i, max=%i, default=%i", lMin, lMax, lDefault);

					if (SUCCEEDED(pProcAmp->Get(VideoProcAmp_Contrast, &Val,  &lCaps)))
						Debug(_log, "Current contrast set to: %i",Val);

					if (SUCCEEDED(pProcAmp->Set(VideoProcAmp_Contrast, _contrast, VideoProcAmp_Flags_Manual)))
						Debug(_log, "Contrast set to: %i",_contrast);
					else
						Error(_log, "Could not set contrast");
				}
				else
					Error(_log, "Contrast is not supported by the grabber");
			}

			if (_saturation != 0)
			{
				if (SUCCEEDED(pProcAmp->GetRange(VideoProcAmp_Saturation, &lMin, &lMax, &lStep, &lDefault, &lCaps)))
				{
					Debug(_log, "Saturation: min=%i, max=%i, default=%i", lMin, lMax, lDefault);

					if (SUCCEEDED(pProcAmp->Get(VideoProcAmp_Saturation, &Val,  &lCaps)))
						Debug(_log, "Current saturation set to: %i",Val);

					if (SUCCEEDED(pProcAmp->Set(VideoProcAmp_Saturation, _saturation, VideoProcAmp_Flags_Manual)))
						Debug(_log, "Saturation set to: %i",_saturation);
					else
						Error(_log, "Could not set saturation");
				}
				else
					Error(_log, "Saturation is not supported by the grabber");
			}

			if (_hue != 0)
			{
				hr = pProcAmp->GetRange(VideoProcAmp_Hue, &lMin, &lMax, &lStep, &lDefault, &lCaps);

				if (SUCCEEDED(hr))
				{
					Debug(_log, "Hue: min=%i, max=%i, default=%i", lMin, lMax, lDefault);

					hr = pProcAmp->Get(VideoProcAmp_Hue, &Val,  &lCaps);
					if (SUCCEEDED(hr))
						Debug(_log, "Current hue set to: %i",Val);

					hr = pProcAmp->Set(VideoProcAmp_Hue, _hue, VideoProcAmp_Flags_Manual);
					if (SUCCEEDED(hr))
						Debug(_log, "Hue set to: %i",_hue);
					else
						Error(_log, "Could not set hue");
				}
				else
					Error(_log, "Hue is not supported by the grabber");
			}
		}
	}

	hr = MFCreateAttributes(&sourceReaderAttributes, 1);
	if(FAILED(hr))
	{
		error = QString("Could not create Source Reader attributes (%1)").arg(hr);
		goto done;
	}

	hr = sourceReaderAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, (IMFSourceReaderCallback *)_sourceReaderCB);
	if(FAILED(hr))
	{
		error = QString("Could not set stream parameter: SetUnknown_MF_SOURCE_READER_ASYNC_CALLBACK (%1)").arg(hr);
		hr = E_INVALIDARG;
		goto done;
	}

	hr = MFCreateSourceReaderFromMediaSource(device, sourceReaderAttributes, &_sourceReader);
	if(FAILED(hr))
	{
		error = QString("Could not create the Source Reader (%1)").arg(hr);
		goto done;
	}

	hr = MFCreateMediaType(&type);
	if(FAILED(hr))
	{
		error = QString("Could not create an empty media type (%1)").arg(hr);
		goto done;
	}

	hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	if(FAILED(hr))
	{
		error = QString("Could not set stream parameter: SetGUID_MF_MT_MAJOR_TYPE (%1)").arg(hr);
		goto done;
	}

	hr = type->SetGUID(MF_MT_SUBTYPE, props.guid);
	if(FAILED(hr))
	{
		error = QString("Could not set stream parameter: SetGUID_MF_MT_SUBTYPE (%1)").arg(hr);
		goto done;
	}

	hr = MFSetAttributeSize(type, MF_MT_FRAME_SIZE, props.x, props.y);
	if(FAILED(hr))
	{
		error = QString("Could not set stream parameter: SMFSetAttributeSize_MF_MT_FRAME_SIZE (%1)").arg(hr);
		goto done;
	}

	hr = MFSetAttributeSize(type, MF_MT_FRAME_RATE, props.fps_a, props.fps_b);
	if(FAILED(hr))
	{
		error = QString("Could not set stream parameter: MFSetAttributeSize_MF_MT_FRAME_RATE (%1)").arg(hr);
		goto done;
	}

	hr = MFSetAttributeRatio(type, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	if(FAILED(hr))
	{
		error = QString("Could not set stream parameter: MFSetAttributeRatio_MF_MT_PIXEL_ASPECT_RATIO (%1)").arg(hr);
		goto done;
	}

	hr = _sourceReaderCB->InitializeVideoEncoder(type, pixelformat);
	if (FAILED(hr))
	{
		error = QString("Failed to initialize the Video Encoder (%1)").arg(hr);
		goto done;
	}

	hr = _sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, type);
	if (FAILED(hr))
	{
		error = QString("Failed to set media type on Source Reader (%1)").arg(hr);
	}

done:
	if(FAILED(hr))
	{
		Error(_log, "%s", QSTRING_CSTR(error));
		SAFE_RELEASE(_sourceReader);
	}
	else
	{
		_pixelFormat = props.pf;
		_width = props.x;
		_height = props.y;
		_frameByteSize = props.x * props.y * 3;
		_lineLength = props.x * 3;
	}

	// Cleanup
	SAFE_RELEASE(deviceAttributes);
	delete[] name;
	SAFE_RELEASE(device);
	SAFE_RELEASE(pProcAmp);
	SAFE_RELEASE(type);
	SAFE_RELEASE(sourceReaderAttributes);

	return hr;
}

void MFGrabber::uninit_device()
{
	SAFE_RELEASE(_sourceReader);
}

void MFGrabber::enumVideoCaptureDevices()
{
	if(FAILED(_hr))
	{
		Error(_log, "enumVideoCaptureDevices(): Media Foundation not initialized");
		return;
	}

	_deviceProperties.clear();

	IMFAttributes* attr;
	if(SUCCEEDED(MFCreateAttributes(&attr, 1)))
	{
		if(SUCCEEDED(attr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
		{
			UINT32 count;
			IMFActivate** devices;
			if(SUCCEEDED(MFEnumDeviceSources(attr, &devices, &count)))
			{
				DebugIf(verbose, _log, "Detected devices: %u", count);
				for(UINT32 i = 0; i < count; i++)
				{
					UINT32 length;
					LPWSTR name;
					LPWSTR symlink;

					if(SUCCEEDED(devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &length)))
					{
						if(SUCCEEDED(devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &symlink, &length)))
						{
							QString dev = QString::fromUtf16((const ushort*)name);
							MFGrabber::DeviceProperties properties;
							properties.name = QString::fromUtf16((const ushort*)symlink);

							Debug(_log, "Found capture device: %s", QSTRING_CSTR(dev));
							IMFMediaSource *pSource = nullptr;
							if(SUCCEEDED(devices[i]->ActivateObject(IID_PPV_ARGS(&pSource))))
							{
								IMFMediaType *pType = nullptr;
								IMFSourceReader* reader;
								if(SUCCEEDED(MFCreateSourceReaderFromMediaSource(pSource, NULL, &reader)))
								{
									for (DWORD i = 0; ; i++)
									{
										if (FAILED(reader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &pType)))
											break;

										GUID format;
										UINT64 frame_size;
										UINT64 frame_rate;

										if( SUCCEEDED(pType->GetGUID(MF_MT_SUBTYPE, &format)) &&
											SUCCEEDED(pType->GetUINT64(MF_MT_FRAME_SIZE, &frame_size)) &&
											SUCCEEDED(pType->GetUINT64(MF_MT_FRAME_RATE, &frame_rate)) &&
											frame_rate > 0)
										{
											PixelFormat pixelformat = GetPixelFormatForGuid(format);
											DWORD w = frame_size >> 32;
											DWORD h = (DWORD) frame_size;
											DWORD fr1 = frame_rate >> 32;
											DWORD fr2 = (DWORD) frame_rate;
											if (pixelformat != PixelFormat::NO_CHANGE)
											{
												int framerate = fr1/fr2;
												QString sFrame = QString::number(framerate).rightJustified(2,' ').trimmed();
												QString displayResolutions = QString::number(w).rightJustified(4,' ').trimmed() +"x"+ QString::number(h).rightJustified(4,' ').trimmed();

												if (!properties.displayResolutions.contains(displayResolutions))
													properties.displayResolutions << displayResolutions;

												if (!properties.framerates.contains(sFrame))
													properties.framerates << sFrame;

												DevicePropertiesItem di;
												di.x = w;
												di.y = h;
												di.fps = framerate;
												di.fps_a = fr1;
												di.fps_b = fr2;
												di.pf = pixelformat;
												di.guid = format;
												properties.valid.append(di);

												DebugIf(verbose, _log,  "%s %d x %d @ %d fps (%s)", QSTRING_CSTR(dev), di.x, di.y, di.fps, QSTRING_CSTR(pixelFormatToString(di.pf)));
											}
										}

										pType->Release();
									}
									reader->Release();
								}
								pSource->Release();
							}

							properties.displayResolutions.sort();
							properties.framerates.sort();
							_deviceProperties.insert(dev, properties);
						}

						CoTaskMemFree(symlink);
					}

					CoTaskMemFree(name);
					devices[i]->Release();
				}

				CoTaskMemFree(devices);
			}

			attr->Release();
		}
	}
}

void MFGrabber::start_capturing()
{
	if (_initialized && _sourceReader)
	{
		HRESULT hr = _sourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
		if (!SUCCEEDED(hr))
			Error(_log, "ReadSample (%i)", hr);
	}
}

void MFGrabber::process_image(const void *frameImageBuffer, int size)
{
	unsigned int processFrameIndex = _currentFrame++;

	// frame skipping
	if((processFrameIndex % _fpsSoftwareDecimation != 0) && (_fpsSoftwareDecimation > 1))
		return;

	// CEC detection
	if(_cecDetectionEnabled)
		return;

	// We do want a new frame...
	if (size < _frameByteSize && _pixelFormat != PixelFormat::MJPEG)
		Error(_log, "Frame too small: %d != %d", size, _frameByteSize);
	else
	{
		if (_threadManager.isActive())
		{
			if (_threadManager._threads == nullptr)
			{
				_threadManager.initThreads();
				Debug(_log, "Max thread count  = %d", _threadManager._maxThreads);

				for (unsigned int i=0; i < _threadManager._maxThreads && _threadManager._threads != nullptr; i++)
				{
					MFThread* _thread = _threadManager._threads[i];
					connect(_thread, SIGNAL(newFrame(unsigned int, const Image<ColorRgb> &,unsigned int)), this , SLOT(newThreadFrame(unsigned int, const Image<ColorRgb> &, unsigned int)));
				}
		    }

			for (unsigned int i = 0;_threadManager.isActive() && i < _threadManager._maxThreads && _threadManager._threads != nullptr; i++)
			{
				if ((_threadManager._threads[i]->isFinished() || !_threadManager._threads[i]->isRunning()))
				{
					// aquire lock
					if ( _threadManager._threads[i]->isBusy() == false)
					{
						MFThread* _thread = _threadManager._threads[i];
						_thread->setup(i, _pixelFormat, (uint8_t *)frameImageBuffer, size, _width, _height, _lineLength, _subsamp, _cropLeft, _cropTop, _cropBottom, _cropRight, _videoMode, _flipMode, processFrameIndex, _pixelDecimation);
						_threadManager._threads[i]->start();
						break;
					}
				}
			}
		}
	}
}

void MFGrabber::setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold, int noSignalCounterThreshold)
{
	_noSignalThresholdColor.red   = uint8_t(255*redSignalThreshold);
	_noSignalThresholdColor.green = uint8_t(255*greenSignalThreshold);
	_noSignalThresholdColor.blue  = uint8_t(255*blueSignalThreshold);
	_noSignalCounterThreshold     = qMax(1, noSignalCounterThreshold);

	if(_signalDetectionEnabled)
		Info(_log, "Signal threshold set to: {%d, %d, %d} and frames: %d", _noSignalThresholdColor.red, _noSignalThresholdColor.green, _noSignalThresholdColor.blue, _noSignalCounterThreshold );
}

void MFGrabber::setSignalDetectionOffset(double horizontalMin, double verticalMin, double horizontalMax, double verticalMax)
{
	// rainbow 16 stripes 0.47 0.2 0.49 0.8
	// unicolor: 0.25 0.25 0.75 0.75

	_x_frac_min = horizontalMin;
	_y_frac_min = verticalMin;
	_x_frac_max = horizontalMax;
	_y_frac_max = verticalMax;

	if(_signalDetectionEnabled)
		Info(_log, "Signal detection area set to: %f,%f x %f,%f", _x_frac_min, _y_frac_min, _x_frac_max, _y_frac_max );
}

bool MFGrabber::start()
{
	try
	{
		if(!_initialized)
		{
			_threadManager.start();
			DebugIf(verbose, _log, "Decoding threads: %d",_threadManager._maxThreads);

			if(init())
			{
				start_capturing();
				Info(_log, "Started");
				return true;
			}
		}
	}
	catch(std::exception& e)
	{
		Error(_log, "Start failed (%s)", e.what());
	}

	return false;
}

void MFGrabber::stop()
{
	if(_initialized)
	{
		_initialized = false;
		_threadManager.stop();
		uninit_device();
		_pixelFormat = _pixelFormatConfig;
		_deviceProperties.clear();
		Info(_log, "Stopped");
	}
}

void MFGrabber::receive_image(const void *frameImageBuffer, int size)
{
	process_image(frameImageBuffer, size);
	start_capturing();
}

void MFGrabber::newThreadFrame(unsigned int threadIndex, const Image<ColorRgb>& image, unsigned int sourceCount)
{
	checkSignalDetectionEnabled(image);

	// get next frame
	if (threadIndex >_threadManager._maxThreads)
		Error(_log, "Frame index %d out of range", sourceCount);

	if (threadIndex <= _threadManager._maxThreads)
		_threadManager._threads[threadIndex]->noBusy();
}

void MFGrabber::checkSignalDetectionEnabled(Image<ColorRgb> image)
{
	if(_signalDetectionEnabled)
	{
		// check signal (only in center of the resulting image, because some grabbers have noise values along the borders)
		bool noSignal = true;

		// top left
		unsigned xOffset  = image.width()  * _x_frac_min;
		unsigned yOffset  = image.height() * _y_frac_min;

		// bottom right
		unsigned xMax     = image.width()  * _x_frac_max;
		unsigned yMax     = image.height() * _y_frac_max;

		for(unsigned x = xOffset; noSignal && x < xMax; ++x)
			for(unsigned y = yOffset; noSignal && y < yMax; ++y)
				noSignal &= (ColorRgb&)image(x, y) <= _noSignalThresholdColor;

		if(noSignal)
			++_noSignalCounter;
		else
		{
			if(_noSignalCounter >= _noSignalCounterThreshold)
			{
				_noSignalDetected = true;
				Info(_log, "Signal detected");
			}

			_noSignalCounter = 0;
		}

		if( _noSignalCounter < _noSignalCounterThreshold)
		{
			emit newFrame(image);
		}
		else if(_noSignalCounter == _noSignalCounterThreshold)
		{
			_noSignalDetected = false;
			Info(_log, "Signal lost");
		}
	}
	else
		emit newFrame(image);
}

QStringList MFGrabber::getV4L2devices() const
{
	QStringList result = QStringList();
	for(auto it = _deviceProperties.begin(); it != _deviceProperties.end(); ++it)
		result << it.key();

	return result;
}

QStringList MFGrabber::getV4L2EncodingFormats(const QString& devicePath) const
{
	QStringList result = QStringList();

	for(int i = 0; i < _deviceProperties[devicePath].valid.count(); ++i )
		if(!result.contains(pixelFormatToString(_deviceProperties[devicePath].valid[i].pf), Qt::CaseInsensitive))
			result << pixelFormatToString(_deviceProperties[devicePath].valid[i].pf).toLower();

	return result;
}

void MFGrabber::setSignalDetectionEnable(bool enable)
{
	if(_signalDetectionEnabled != enable)
	{
		_signalDetectionEnabled = enable;
		Info(_log, "Signal detection is now %s", enable ? "enabled" : "disabled");
	}
}

void MFGrabber::setCecDetectionEnable(bool enable)
{
	if(_cecDetectionEnabled != enable)
	{
		_cecDetectionEnabled = enable;
		Info(_log, QString("CEC detection is now %1").arg(enable ? "enabled" : "disabled").toLocal8Bit());
	}
}

bool MFGrabber::setDevice(QString device)
{
	if(_currentDeviceName != device)
	{
		_currentDeviceName = device;
		return true;
	}
	return false;
}

void MFGrabber::setPixelDecimation(int pixelDecimation)
{
	if(_pixelDecimation != pixelDecimation)
		_pixelDecimation = pixelDecimation;
}

void MFGrabber::setFlipMode(QString flipMode)
{
	if(_flipMode != parseFlipMode(flipMode))
		Grabber::setFlipMode(parseFlipMode(flipMode));
}

bool MFGrabber::setWidthHeight(int width, int height)
{
	if(Grabber::setWidthHeight(width, height))
	{
		Debug(_log,"Set device resolution to width: %i, height: %i", width, height);
		return true;
	}
	else if(width == 0 && height == 0)
	{
		_width = _height = 0;
		Debug(_log,"Set device resolution to 'Automatic'");
		return true;
	}

	return false;
}

bool MFGrabber::setFramerate(int fps)
{
	if(Grabber::setFramerate(fps))
	{
		Debug(_log,"Set fps to: %i", fps);
		return true;
	}
	return false;
}

void MFGrabber::setFpsSoftwareDecimation(int decimation)
{
	_fpsSoftwareDecimation = decimation;
	if(decimation > 1)
		Debug(_log,"Every %ith image per second are processed", decimation);
}

bool MFGrabber::setEncoding(QString enc)
{
	if(_pixelFormatConfig != parsePixelFormat(enc))
	{
		Debug(_log,"Set hardware encoding to: %s", QSTRING_CSTR(enc.toUpper()));
		_pixelFormatConfig = parsePixelFormat(enc);
		if(!_initialized)
			_pixelFormat = _pixelFormatConfig;
		return true;
	}
	return false;
}

bool MFGrabber::setBrightnessContrastSaturationHue(int brightness, int contrast, int saturation, int hue)
{
	if(_brightness != brightness || _contrast != contrast || _saturation != saturation || _hue != hue)
	{
		_brightness = brightness;
		_contrast = contrast;
		_saturation = saturation;
		_hue = hue;

		Debug(_log,"Set brightness to %i, contrast to %i, saturation to %i, hue to %i", _brightness, _contrast, _saturation, _hue);
		return true;
	}
	return false;
}

void MFGrabber::reloadGrabber()
{
	if(_initialized)
	{
		Debug(_log,"Reloading Media Foundation Grabber");
		// stop grabber
		uninit();
		// restore pixelformat to configs value
		_pixelFormat = _pixelFormatConfig;
		// set _newDeviceName
		_newDeviceName = _currentDeviceName;
		// start grabber
		start();
	}
}
