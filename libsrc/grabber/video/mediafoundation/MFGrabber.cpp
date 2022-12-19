#include "MFSourceReaderCB.h"
#include "grabber/MFGrabber.h"

// Constants
namespace { const bool verbose = false; }

// Need more video properties? Visit https://docs.microsoft.com/en-us/windows/win32/api/strmif/ne-strmif-videoprocampproperty
using VideoProcAmpPropertyMap = QMap<VideoProcAmpProperty, QString>;
inline QMap<VideoProcAmpProperty, QString> initVideoProcAmpPropertyMap()
{
	QMap<VideoProcAmpProperty, QString> propertyMap
	{
		{VideoProcAmp_Brightness, "brightness"	},
		{VideoProcAmp_Contrast	, "contrast"	},
		{VideoProcAmp_Saturation, "saturation"	},
		{VideoProcAmp_Hue		, "hue"			}
	};

	return propertyMap;
};

Q_GLOBAL_STATIC_WITH_ARGS(VideoProcAmpPropertyMap, _videoProcAmpPropertyMap, (initVideoProcAmpPropertyMap()));

MFGrabber::MFGrabber()
	: Grabber("V4L2:MEDIA_FOUNDATION")
	, _currentDeviceName("none")
	, _newDeviceName("none")
	, _hr(S_FALSE)
	, _sourceReader(nullptr)
	, _sourceReaderCB(nullptr)
	, _threadManager(nullptr)
	, _pixelFormat(PixelFormat::NO_CHANGE)
	, _pixelFormatConfig(PixelFormat::NO_CHANGE)
	, _lineLength(-1)
	, _frameByteSize(-1)
	, _noSignalCounterThreshold(40)
	, _noSignalCounter(0)
	, _brightness(0)
	, _contrast(0)
	, _saturation(0)
	, _hue(0)
	, _currentFrame(0)
	, _noSignalThresholdColor(ColorRgb{0,0,0})
	, _signalDetectionEnabled(true)
	, _noSignalDetected(false)
	, _initialized(false)
	, _reload(false)
	, _x_frac_min(0.25)
	, _y_frac_min(0.25)
	, _x_frac_max(0.75)
	, _y_frac_max(0.75)
{
	CoInitializeEx(0, COINIT_MULTITHREADED);
	_hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	if (FAILED(_hr))
		CoUninitialize();
}

MFGrabber::~MFGrabber()
{
	uninit();

	SAFE_RELEASE(_sourceReader);

	if (_sourceReaderCB != nullptr)
		while (_sourceReaderCB->isBusy()) {}

	SAFE_RELEASE(_sourceReaderCB);

	if (_threadManager)
		delete _threadManager;
	_threadManager = nullptr;

	if (SUCCEEDED(_hr) && SUCCEEDED(MFShutdown()))
		CoUninitialize();
}

bool MFGrabber::prepare()
{
	if (SUCCEEDED(_hr))
	{
		if (!_sourceReaderCB)
			_sourceReaderCB = new SourceReaderCB(this);

		if (!_threadManager)
			_threadManager = new EncoderThreadManager(this);

		return (_sourceReaderCB != nullptr && _threadManager != nullptr);
	}

	return false;
}

bool MFGrabber::start()
{
	if (!_initialized)
	{
		if (init())
		{
			connect(_threadManager, &EncoderThreadManager::newFrame, this, &MFGrabber::newThreadFrame);
			_threadManager->start();
			DebugIf(verbose, _log, "Decoding threads: %d", _threadManager->_threadCount);

			start_capturing();
			Info(_log, "Started");
			return true;
		}
		else
		{
			Error(_log, "The Media Foundation Grabber could not be started");
			return false;
		}
	}
	else
		return true;
}

void MFGrabber::stop()
{
	if (_initialized)
	{
		_initialized = false;
		_threadManager->stop();
		disconnect(_threadManager, nullptr, nullptr, nullptr);
		_sourceReader->Flush(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
		SAFE_RELEASE(_sourceReader);
		_deviceProperties.clear();
		_deviceControls.clear();
		Info(_log, "Stopped");
	}
}

bool MFGrabber::init()
{
	// enumerate the video capture devices on the user's system
	enumVideoCaptureDevices();

	if (!_initialized && SUCCEEDED(_hr))
	{
		int deviceIndex = -1;
		bool noDeviceName = _currentDeviceName.compare("none", Qt::CaseInsensitive) == 0 || _currentDeviceName.compare("auto", Qt::CaseInsensitive) == 0;

		if (noDeviceName)
			return false;

		if (!_deviceProperties.contains(_currentDeviceName))
		{
			Debug(_log, "Configured device '%s' is not available.", QSTRING_CSTR(_currentDeviceName));
			return false;
		}

		Debug(_log,  "Searching for %s %d x %d @ %d fps (%s)", QSTRING_CSTR(_currentDeviceName), _width, _height,_fps, QSTRING_CSTR(pixelFormatToString(_pixelFormat)));

		QList<DeviceProperties> dev = _deviceProperties[_currentDeviceName];
		for ( int i = 0; i < dev.count() && deviceIndex < 0; ++i )
		{
			if (dev[i].width != _width || dev[i].height != _height || dev[i].fps != _fps || dev[i].pf != _pixelFormat)
				continue;
			else
				deviceIndex = i;
		}

		if (deviceIndex >= 0 && SUCCEEDED(init_device(_currentDeviceName, dev[deviceIndex])))
		{
			_initialized = true;
			_newDeviceName = _currentDeviceName;
		}
		else
		{
			Debug(_log, "Configured device '%s' is not available.", QSTRING_CSTR(_currentDeviceName));
			return false;
		}

	}
	return _initialized;
}

void MFGrabber::uninit()
{
	// stop if the grabber was not stopped
	if (_initialized)
	{
		Debug(_log,"Uninit grabber: %s", QSTRING_CSTR(_newDeviceName));
		stop();
	}
}

HRESULT MFGrabber::init_device(QString deviceName, DeviceProperties props)
{
	PixelFormat pixelformat = GetPixelFormatForGuid(props.guid);
	QString error;
	IMFMediaSource* device = nullptr;
	IMFAttributes* deviceAttributes = nullptr, *sourceReaderAttributes = nullptr;
	IMFMediaType* type = nullptr;
	HRESULT hr = S_OK;
	IAMVideoProcAmp* pProcAmp = nullptr;

	Debug(_log, "Init %s, %d x %d @ %d fps (%s)", QSTRING_CSTR(deviceName), props.width, props.height, props.fps, QSTRING_CSTR(pixelFormatToString(pixelformat)));
	DebugIf (verbose, _log, "Symbolic link: %s", QSTRING_CSTR(props.symlink));

	hr = MFCreateAttributes(&deviceAttributes, 2);
	if (FAILED(hr))
	{
		error = QString("Could not create device attributes (%1)").arg(hr);
		goto done;
	}

	hr = deviceAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (FAILED(hr))
	{
		error = QString("SetGUID_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE (%1)").arg(hr);
		goto done;
	}

	if (FAILED(deviceAttributes->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, (LPCWSTR)props.symlink.utf16())))
	{
		error = QString("IMFAttributes_SetString_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK (%1)").arg(hr);
		goto done;
	}

	hr = MFCreateDeviceSource(deviceAttributes, &device);
	if (FAILED(hr))
	{
		error = QString("MFCreateDeviceSource (%1)").arg(hr);
		goto done;
	}

	if (!device)
	{
		error = QString("Could not open device (%1)").arg(hr);
		goto done;
	}
	else
		Debug(_log, "Device opened");

	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&pProcAmp))))
	{
		for (auto control : _deviceControls[deviceName])
		{
			switch (_videoProcAmpPropertyMap->key(control.property))
			{
				case VideoProcAmpProperty::VideoProcAmp_Brightness:
					if (_brightness >= control.minValue && _brightness <= control.maxValue && _brightness != control.currentValue)
					{
						Debug(_log,"Set brightness to %i", _brightness);
						pProcAmp->Set(VideoProcAmp_Brightness, _brightness, VideoProcAmp_Flags_Manual);
					}
				break;
				case VideoProcAmpProperty::VideoProcAmp_Contrast:
					if (_contrast >= control.minValue && _contrast <= control.maxValue && _contrast != control.currentValue)
					{
						Debug(_log,"Set contrast to %i", _contrast);
						pProcAmp->Set(VideoProcAmp_Contrast, _contrast, VideoProcAmp_Flags_Manual);
					}
				break;
				case VideoProcAmpProperty::VideoProcAmp_Saturation:
					if (_saturation >= control.minValue && _saturation <= control.maxValue && _saturation != control.currentValue)
					{
						Debug(_log,"Set saturation to %i", _saturation);
						pProcAmp->Set(VideoProcAmp_Saturation, _saturation, VideoProcAmp_Flags_Manual);
					}
				break;
				case VideoProcAmpProperty::VideoProcAmp_Hue:
					if (_hue >= control.minValue && _hue <= control.maxValue && _hue != control.currentValue)
					{
						Debug(_log,"Set hue to %i", _hue);
						pProcAmp->Set(VideoProcAmp_Hue, _hue, VideoProcAmp_Flags_Manual);
					}
				break;
				default:
					break;
			}
		}
	}

	hr = MFCreateAttributes(&sourceReaderAttributes, 1);
	if (FAILED(hr))
	{
		error = QString("Could not create Source Reader attributes (%1)").arg(hr);
		goto done;
	}

	hr = sourceReaderAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, (IMFSourceReaderCallback *)_sourceReaderCB);
	if (FAILED(hr))
	{
		error = QString("Could not set stream parameter: SetUnknown_MF_SOURCE_READER_ASYNC_CALLBACK (%1)").arg(hr);
		hr = E_INVALIDARG;
		goto done;
	}

	hr = MFCreateSourceReaderFromMediaSource(device, sourceReaderAttributes, &_sourceReader);
	if (FAILED(hr))
	{
		error = QString("Could not create the Source Reader (%1)").arg(hr);
		goto done;
	}

	hr = MFCreateMediaType(&type);
	if (FAILED(hr))
	{
		error = QString("Could not create an empty media type (%1)").arg(hr);
		goto done;
	}

	hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	if (FAILED(hr))
	{
		error = QString("Could not set stream parameter: SetGUID_MF_MT_MAJOR_TYPE (%1)").arg(hr);
		goto done;
	}

	hr = type->SetGUID(MF_MT_SUBTYPE, props.guid);
	if (FAILED(hr))
	{
		error = QString("Could not set stream parameter: SetGUID_MF_MT_SUBTYPE (%1)").arg(hr);
		goto done;
	}

	hr = MFSetAttributeSize(type, MF_MT_FRAME_SIZE, props.width, props.height);
	if (FAILED(hr))
	{
		error = QString("Could not set stream parameter: SMFSetAttributeSize_MF_MT_FRAME_SIZE (%1)").arg(hr);
		goto done;
	}

	hr = MFSetAttributeSize(type, MF_MT_FRAME_RATE, props.numerator, props.denominator);
	if (FAILED(hr))
	{
		error = QString("Could not set stream parameter: MFSetAttributeSize_MF_MT_FRAME_RATE (%1)").arg(hr);
		goto done;
	}

	hr = MFSetAttributeRatio(type, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	if (FAILED(hr))
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
	if (FAILED(hr))
	{
		emit readError(QSTRING_CSTR(error));
		SAFE_RELEASE(_sourceReader);
	}
	else
	{
		_pixelFormat = props.pf;
		_width = props.width;
		_height = props.height;
		_frameByteSize = _width * _height * 3;
		_lineLength = _width * 3;
	}

	// Cleanup
	SAFE_RELEASE(deviceAttributes);
	SAFE_RELEASE(device);
	SAFE_RELEASE(pProcAmp);
	SAFE_RELEASE(type);
	SAFE_RELEASE(sourceReaderAttributes);

	return hr;
}

void MFGrabber::enumVideoCaptureDevices()
{
	_deviceProperties.clear();
	_deviceControls.clear();

	IMFAttributes* attr;
	if (SUCCEEDED(MFCreateAttributes(&attr, 1)))
	{
		if (SUCCEEDED(attr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
		{
			UINT32 count;
			IMFActivate** devices;
			if (SUCCEEDED(MFEnumDeviceSources(attr, &devices, &count)))
			{
				DebugIf (verbose, _log, "Detected devices: %u", count);
				for (UINT32 i = 0; i < count; i++)
				{
					UINT32 length;
					LPWSTR name;
					LPWSTR symlink;

					if (SUCCEEDED(devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &length)))
					{
						if (SUCCEEDED(devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &symlink, &length)))
						{
							QList<DeviceProperties> devicePropertyList;
							QString dev = QString::fromUtf16((const char16_t*)name);

							IMFMediaSource *pSource = nullptr;
							if (SUCCEEDED(devices[i]->ActivateObject(IID_PPV_ARGS(&pSource))))
							{
								DebugIf (verbose, _log, "Found capture device: %s", QSTRING_CSTR(dev));

								IMFMediaType *pType = nullptr;
								IMFSourceReader* reader;
								if (SUCCEEDED(MFCreateSourceReaderFromMediaSource(pSource, NULL, &reader)))
								{
									for (DWORD i = 0; ; i++)
									{
										if (FAILED(reader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &pType)))
											break;

										GUID format;
										UINT32 width = 0, height = 0, numerator = 0, denominator = 0;

										if ( SUCCEEDED(pType->GetGUID(MF_MT_SUBTYPE, &format)) &&
											SUCCEEDED(MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height)) &&
											SUCCEEDED(MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, &numerator, &denominator)))
										{
											PixelFormat pixelformat = GetPixelFormatForGuid(format);
											if (pixelformat != PixelFormat::NO_CHANGE)
											{
												DeviceProperties properties;
												properties.symlink = QString::fromUtf16((const char16_t*)symlink);
												properties.width = width;
												properties.height = height;
												properties.fps = numerator / denominator;
												properties.numerator = numerator;
												properties.denominator = denominator;
												properties.pf = pixelformat;
												properties.guid = format;
												devicePropertyList.append(properties);

												DebugIf (verbose, _log, "%s %d x %d @ %d fps (%s)", QSTRING_CSTR(dev), properties.width, properties.height, properties.fps, QSTRING_CSTR(pixelFormatToString(properties.pf)));
											}
										}

										SAFE_RELEASE(pType);
									}

									IAMVideoProcAmp *videoProcAmp = nullptr;
									if (SUCCEEDED(pSource->QueryInterface(IID_PPV_ARGS(&videoProcAmp))))
									{
										QList<DeviceControls> deviceControlList;
										for (auto it = _videoProcAmpPropertyMap->begin(); it != _videoProcAmpPropertyMap->end(); it++)
										{
											long minVal, maxVal, stepVal, defaultVal, flag;
											if (SUCCEEDED(videoProcAmp->GetRange(it.key(), &minVal, &maxVal, &stepVal, &defaultVal, &flag)))
											{
												if (flag & VideoProcAmp_Flags_Manual)
												{
													DeviceControls control;
													control.property = it.value();
													control.minValue = minVal;
													control.maxValue = maxVal;
													control.step = stepVal;
													control.def = defaultVal;

													long currentVal;
													if (SUCCEEDED(videoProcAmp->Get(it.key(), &currentVal,  &flag)))
													{
														control.currentValue = currentVal;
														DebugIf(verbose, _log, "%s: min=%i, max=%i, step=%i, default=%i, current=%i", QSTRING_CSTR(it.value()), minVal, maxVal, stepVal, defaultVal, currentVal);
													}
													else
														break;

													deviceControlList.append(control);
												}
											}

										}

										if (!deviceControlList.isEmpty())
											_deviceControls.insert(dev, deviceControlList);
									}

									SAFE_RELEASE(videoProcAmp);
									SAFE_RELEASE(reader);
								}

								SAFE_RELEASE(pSource);
							}

							if (!devicePropertyList.isEmpty())
								_deviceProperties.insert(dev, devicePropertyList);
						}

						CoTaskMemFree(symlink);
					}

					CoTaskMemFree(name);
					SAFE_RELEASE(devices[i]);
				}

				CoTaskMemFree(devices);
			}

			SAFE_RELEASE(attr);
		}
	}
}

void MFGrabber::start_capturing()
{
	if (_initialized && _sourceReader && _threadManager)
	{
		HRESULT hr = _sourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
		if (!SUCCEEDED(hr))
		{
			Error(_log, "ReadSample (%i)", hr);
		}
	}
}

void MFGrabber::process_image(const void *frameImageBuffer, int size)
{
	int processFrameIndex = _currentFrame++;

	// frame skipping
	if ((processFrameIndex % (_fpsSoftwareDecimation + 1) != 0) && (_fpsSoftwareDecimation > 0))
		return;

	// We do want a new frame...
#ifdef HAVE_TURBO_JPEG
	if (size < _frameByteSize && _pixelFormat != PixelFormat::MJPEG)
#else
	if (size < _frameByteSize)
#endif
		Error(_log, "Frame too small: %d != %d", size, _frameByteSize);
	else if (_threadManager != nullptr)
	{
		for (unsigned long i = 0; i < _threadManager->_threadCount; i++)
		{
			if (!_threadManager->_threads[i]->isBusy())
			{
				_threadManager->_threads[i]->setup(_pixelFormat, (uint8_t*)frameImageBuffer, size, _width, _height, _lineLength, _cropLeft, _cropTop, _cropBottom, _cropRight, _videoMode, _flipMode, _pixelDecimation);
				_threadManager->_threads[i]->process();
				break;
			}
		}
	}
}

void MFGrabber::receive_image(const void *frameImageBuffer, int size)
{
	process_image(frameImageBuffer, size);
	start_capturing();
}

void MFGrabber::newThreadFrame(Image<ColorRgb> image)
{
	if (_signalDetectionEnabled)
	{
		// check signal (only in center of the resulting image, because some grabbers have noise values along the borders)
		bool noSignal = true;

		// top left
		unsigned xOffset  = image.width()  * _x_frac_min;
		unsigned yOffset  = image.height() * _y_frac_min;

		// bottom right
		unsigned xMax     = image.width()  * _x_frac_max;
		unsigned yMax     = image.height() * _y_frac_max;

		for (unsigned x = xOffset; noSignal && x < xMax; ++x)
			for (unsigned y = yOffset; noSignal && y < yMax; ++y)
				noSignal &= (ColorRgb&)image(x, y) <= _noSignalThresholdColor;

		if (noSignal)
			++_noSignalCounter;
		else
		{
			if (_noSignalCounter >= _noSignalCounterThreshold)
			{
				_noSignalDetected = true;
				Info(_log, "Signal detected");
			}

			_noSignalCounter = 0;
		}

		if ( _noSignalCounter < _noSignalCounterThreshold)
		{
			emit newFrame(image);
		}
		else if (_noSignalCounter == _noSignalCounterThreshold)
		{
			_noSignalDetected = false;
			Info(_log, "Signal lost");
		}
	}
	else
		emit newFrame(image);
}

void MFGrabber::setDevice(const QString& device)
{
	if (_currentDeviceName != device)
	{
		_currentDeviceName = device;
		_reload = true;
	}
}

bool MFGrabber::setInput(int input)
{
	if (Grabber::setInput(input))
	{
		_reload = true;
		return true;
	}

	 return false;
}

bool MFGrabber::setWidthHeight(int width, int height)
{
	if (Grabber::setWidthHeight(width, height))
	{
		_reload = true;
		return true;
	}

	 return false;
}

void MFGrabber::setEncoding(QString enc)
{
	if (_pixelFormatConfig != parsePixelFormat(enc))
	{
		_pixelFormatConfig = parsePixelFormat(enc);
		if (_initialized)
		{
			Debug(_log,"Set hardware encoding to: %s", QSTRING_CSTR(enc.toUpper()));
			_reload = true;
		}
		else
			_pixelFormat = _pixelFormatConfig;
	}
}

void MFGrabber::setBrightnessContrastSaturationHue(int brightness, int contrast, int saturation, int hue)
{
	if (_brightness != brightness || _contrast != contrast || _saturation != saturation || _hue != hue)
	{
		_brightness = brightness;
		_contrast = contrast;
		_saturation = saturation;
		_hue = hue;

		_reload = true;
	}
}

void MFGrabber::setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold, int noSignalCounterThreshold)
{
	_noSignalThresholdColor.red   = uint8_t(255*redSignalThreshold);
	_noSignalThresholdColor.green = uint8_t(255*greenSignalThreshold);
	_noSignalThresholdColor.blue  = uint8_t(255*blueSignalThreshold);
	_noSignalCounterThreshold     = qMax(1, noSignalCounterThreshold);

	if (_signalDetectionEnabled)
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

	if (_signalDetectionEnabled)
		Info(_log, "Signal detection area set to: %f,%f x %f,%f", _x_frac_min, _y_frac_min, _x_frac_max, _y_frac_max );
}

void MFGrabber::setSignalDetectionEnable(bool enable)
{
	if (_signalDetectionEnabled != enable)
	{
		_signalDetectionEnabled = enable;
		if (_initialized)
			Info(_log, "Signal detection is now %s", enable ? "enabled" : "disabled");
	}
}

bool MFGrabber::reload(bool force)
{
	if (_reload || force)
	{
		if (_sourceReader)
		{
			Info(_log,"Reloading Media Foundation Grabber");
			uninit();
			_pixelFormat = _pixelFormatConfig;
			_newDeviceName = _currentDeviceName;
		}

		_reload = false;
		return prepare() && start();
	}

	return false;
}

QJsonArray MFGrabber::discover(const QJsonObject& params)
{
	DebugIf (verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	enumVideoCaptureDevices();

	QJsonArray inputsDiscovered;
	for (auto it = _deviceProperties.begin(); it != _deviceProperties.end(); ++it)
	{
		QJsonObject device, in;
		QJsonArray video_inputs, formats;

		device["device"] = it.key();
		device["device_name"] = it.key();
		device["type"] = "v4l2";

		in["name"] = "";
		in["inputIdx"] = 0;

		QStringList encodingFormats = QStringList();
		for (int i = 0; i < _deviceProperties[it.key()].count(); ++i )
			if (!encodingFormats.contains(pixelFormatToString(_deviceProperties[it.key()][i].pf), Qt::CaseInsensitive))
				encodingFormats << pixelFormatToString(_deviceProperties[it.key()][i].pf).toLower();

		for (auto encodingFormat : encodingFormats)
		{
			QJsonObject format;
			QJsonArray resolutionArray;

			format["format"] = encodingFormat;

			QMultiMap<int, int> deviceResolutions = QMultiMap<int, int>();
			for (int i = 0; i < _deviceProperties[it.key()].count(); ++i )
				if (!deviceResolutions.contains(_deviceProperties[it.key()][i].width, _deviceProperties[it.key()][i].height) && _deviceProperties[it.key()][i].pf == parsePixelFormat(encodingFormat))
					deviceResolutions.insert(_deviceProperties[it.key()][i].width, _deviceProperties[it.key()][i].height);

			for (auto width_height = deviceResolutions.begin(); width_height != deviceResolutions.end(); width_height++)
			{
				QJsonObject resolution;
				QJsonArray fps;

				resolution["width"] = width_height.key();
				resolution["height"] = width_height.value();

				QIntList framerates = QIntList();
				for (int i = 0; i < _deviceProperties[it.key()].count(); ++i )
				{
					int fps = _deviceProperties[it.key()][i].numerator / _deviceProperties[it.key()][i].denominator;
					if (!framerates.contains(fps) && _deviceProperties[it.key()][i].pf == parsePixelFormat(encodingFormat) && _deviceProperties[it.key()][i].width == width_height.key() && _deviceProperties[it.key()][i].height == width_height.value())
						framerates << fps;
				}

				for (auto framerate : framerates)
					fps.append(framerate);

				resolution["fps"] = fps;
				resolutionArray.append(resolution);
			}

			format["resolutions"] = resolutionArray;
			formats.append(format);
		}
		in["formats"] = formats;
		video_inputs.append(in);
		device["video_inputs"] = video_inputs;

		QJsonObject controls, controls_default;
		for (auto control : _deviceControls[it.key()])
		{
			QJsonObject property;
			property["minValue"] = control.minValue;
			property["maxValue"] = control.maxValue;
			property["step"] = control.step;
			property["current"] = control.currentValue;
			controls[control.property] = property;
			controls_default[control.property] = control.def;
		}
		device["properties"] = controls;

		QJsonObject defaults, video_inputs_default, format_default, resolution_default;
		resolution_default["width"] = 640;
		resolution_default["height"] = 480;
		resolution_default["fps"] = 25;
		format_default["format"] = "bgr24";
		format_default["resolution"] = resolution_default;
		video_inputs_default["inputIdx"] = 0;
		video_inputs_default["standards"] = "PAL";
		video_inputs_default["formats"] = format_default;

		defaults["video_input"] = video_inputs_default;
		defaults["properties"] = controls_default;
		device["default"] = defaults;

		inputsDiscovered.append(device);
	}

	_deviceProperties.clear();
	_deviceControls.clear();
	DebugIf (verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;
}
