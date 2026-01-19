#include <grabber/drm/DRMFrameGrabber.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string>
#include <iostream>
#include <vector>

#include <QThread>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>
#include <QSize>
#include <QMap>

// Add missing AMD format modifier definitions for downward compatibility
#ifndef AMD_FMT_MOD_TILE_VER_GFX11
#define AMD_FMT_MOD_TILE_VER_GFX11 4
#endif
#ifndef AMD_FMT_MOD_TILE_VER_GFX12
#define AMD_FMT_MOD_TILE_VER_GFX12 5
#endif


#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define ALIGN(v, a) (((v) + (a) - 1) & ~((a) - 1))

static QString getDrmFormat(uint32_t format)
{
    return QString::fromLatin1(reinterpret_cast<const char*>(&format), sizeof format);
}

static QString getDrmModifierName(uint64_t modifier)
{
    uint64_t vendor = modifier >> 56;
    uint64_t mod = modifier & 0x00FFFFFFFFFFFFFF;
    QString name = QString("VENDOR: 0x%1, MOD: 0x%2").arg(vendor, 2, 16, QChar('0')).arg(mod, 14, 16, QChar('0'));

    switch (modifier)
    {
    case DRM_FORMAT_MOD_INVALID:
        return "DRM_FORMAT_MOD_INVALID";
    case DRM_FORMAT_MOD_LINEAR:
        return "DRM_FORMAT_MOD_LINEAR";
    default:
        break;
    }

    switch (vendor)
    {
    case DRM_FORMAT_MOD_VENDOR_INTEL:
        switch (mod)
        {
        case I915_FORMAT_MOD_X_TILED:
            return "I915_FORMAT_MOD_X_TILED";
        case I915_FORMAT_MOD_Y_TILED:
            return "I915_FORMAT_MOD_Y_TILED";
        case I915_FORMAT_MOD_Yf_TILED:
            return "I915_FORMAT_MOD_Yf_TILED";
        case I915_FORMAT_MOD_Y_TILED_CCS:
            return "I915_FORMAT_MOD_Y_TILED_CCS";
        case I915_FORMAT_MOD_Yf_TILED_CCS:
            return "I915_FORMAT_MOD_Yf_TILED_CCS";
        default:
            return QString("DRM_FORMAT_MOD_INTEL_UNKNOWN [0x%1]").arg(mod, 14, 16, QChar('0'));
        }
    case DRM_FORMAT_MOD_VENDOR_AMD:
        if (mod & AMD_FMT_MOD_TILE_VER_GFX9)
            return "AMD_FMT_MOD_TILE_VER_GFX9";
        if (mod & AMD_FMT_MOD_TILE_VER_GFX10)
            return "AMD_FMT_MOD_TILE_VER_GFX10";
        if (mod & AMD_FMT_MOD_TILE_VER_GFX11)
            return "AMD_FMT_MOD_TILE_VER_GFX11";
        if (mod & AMD_FMT_MOD_TILE_VER_GFX12)
            return "AMD_FMT_MOD_TILE_VER_GFX12";
        if (mod & AMD_FMT_MOD_DCC_BLOCK_128B)
            return "AMD_FMT_MOD_DCC_BLOCK_128B";
        if (mod & AMD_FMT_MOD_DCC_BLOCK_256B)
            return "AMD_FMT_MOD_DCC_BLOCK_256B";
        return QString("DRM_FORMAT_MOD_AMD_UNKNOWN [0x%1]").arg(mod, 14, 16, QChar('0'));
    case DRM_FORMAT_MOD_VENDOR_NVIDIA:
        if (mod & 0x10)
            return "DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D";
        return QString("DRM_FORMAT_MOD_NVIDIA_UNKNOWN [0x%1]").arg(mod , 14, 16, QChar('0'));
    case DRM_FORMAT_MOD_VENDOR_BROADCOM:
        switch (fourcc_mod_broadcom_mod(modifier))
        {
        case DRM_FORMAT_MOD_BROADCOM_SAND32:
            return "DRM_FORMAT_MOD_BROADCOM_SAND32";
        case DRM_FORMAT_MOD_BROADCOM_SAND64:
            return "DRM_FORMAT_MOD_BROADCOM_SAND64";
        case DRM_FORMAT_MOD_BROADCOM_SAND128:
            return "DRM_FORMAT_MOD_BROADCOM_SAND128";
        case DRM_FORMAT_MOD_BROADCOM_SAND256:
            return "DRM_FORMAT_MOD_BROADCOM_SAND256";
        case DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED:
            return "DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED";
        default:
            return QString("DRM_FORMAT_MOD_BROADCOM_UNKNOWN [0x%1]").arg(mod, 14, 16, QChar('0'));
        }
    case DRM_FORMAT_MOD_VENDOR_ARM:
        if ((modifier & DRM_FORMAT_MOD_ARM_AFBC(0)) == DRM_FORMAT_MOD_ARM_AFBC(0))
            return "DRM_FORMAT_MOD_ARM_AFBC";
        return QString("DRM_FORMAT_MOD_ARM_UNKNOWN [0x%1]").arg(mod, 14, 16, QChar('0'));
        
    default:
        break;
    }

    return name;
}

static PixelFormat GetPixelFormatForDrmFormat(uint32_t format)
{
    switch (format)
    {
#ifdef DRM_FORMAT_RGB565
    case DRM_FORMAT_RGB565: return PixelFormat::BGR16;
#endif
    case DRM_FORMAT_XRGB8888: return PixelFormat::BGR32;
    case DRM_FORMAT_ARGB8888: return PixelFormat::BGR32;
    case DRM_FORMAT_XBGR8888: return PixelFormat::RGB32;
    case DRM_FORMAT_ABGR8888: return PixelFormat::RGB32;
    case DRM_FORMAT_NV12:     return PixelFormat::NV12;
#ifdef DRM_FORMAT_NV21
    case DRM_FORMAT_NV21:     return PixelFormat::NV21;
#endif
#ifdef DRM_FORMAT_P030
    case DRM_FORMAT_P030:     return PixelFormat::P030;
#endif
    case DRM_FORMAT_YUV420:   return PixelFormat::I420;
    default:                  return PixelFormat::NO_CHANGE;
    }
}

// Code from: https://gitlab.freedesktop.org/drm/igt-gpu-tools/-/blob/master/lib/igt_vc4.c#L339
static size_t vc4_sand_tiled_offset(size_t column_width, size_t column_size, size_t x, size_t y, size_t bpp)
{
    size_t offset = 0;
    size_t cols_x;
    size_t pix_x;

    /* Offset to the beginning of the relevant column. */
    cols_x = x / column_width;
    offset += cols_x * column_size;

    /* Offset to the relevant pixel. */
    pix_x = x % column_width;
    offset += (column_width * y + pix_x) * bpp / 8;

    return offset;
}

// Forward declarations for QDebug operators
QDebug operator<<(QDebug dbg, const drmModeFB2* fb);
QDebug operator<<(QDebug dbg, const drmModePlane* plane);

DRMFrameGrabber::DRMFrameGrabber(int deviceIdx, int cropLeft, int cropRight, int cropTop, int cropBottom)
    : Grabber("GRABBER-DRM", cropLeft, cropRight, cropTop, cropBottom), _deviceFd(-1), _crtc(nullptr)
{
    _input = deviceIdx;
    _useImageResampler = true;
}

DRMFrameGrabber::~DRMFrameGrabber()
{
    freeResources();
    closeDevice();
}

void DRMFrameGrabber::freeResources()
{
    _connectors.clear();
    _encoders.clear();

    if (_crtc != nullptr)
    {
        drmModeFreeCrtc(_crtc);
        _crtc = nullptr;
    }

    for (auto const &[id, plane] : _planes)
    {
        if (plane != nullptr)
        {
            drmModeFreePlane(plane);
        }
    }
    _planes.clear();

    for (auto const &[id, framebuffer] : _framebuffers)
    {
        drmModeFreeFB2(framebuffer);
    }
    _framebuffers.clear();
}

bool DRMFrameGrabber::setupScreen()
{
    freeResources();
    closeDevice();

    bool success = openDevice() && getScreenInfo();
    setEnabled(success);

    if (!success)
    {
        freeResources();
        closeDevice();
    }

    return success;
}

bool DRMFrameGrabber::setWidthHeight(int width, int height)
{
    if (Grabber::setWidthHeight(width, height))
    {
        return setupScreen();
    }

    return false;
}

struct LinearFramebufferParams
{
    int deviceFd;
    const drmModeFB2* framebuffer;
    int w;
    int h;
    PixelFormat pixelFormat;
    const ImageResampler& imageResampler;
    QSharedPointer<Logger> log;
    Image<ColorRgb>& image;
};

static bool processLinearFramebuffer(const LinearFramebufferParams& params)
{
    int size = 0;
    int lineLength = 0;
    int fb_dmafd = 0;

    if (params.pixelFormat == PixelFormat::I420 || params.pixelFormat == PixelFormat::NV12
#ifdef DRM_FORMAT_NV21
        || params.pixelFormat == PixelFormat::NV21
#endif
    )
    {
        size = (params.w * params.h * 3) / 2;
        lineLength = params.w;
    }
#ifdef DRM_FORMAT_P030
    else if (params.pixelFormat == PixelFormat::P030)
    {
        size = (params.w * params.h * 2) + (DIV_ROUND_UP(params.w, 2) * DIV_ROUND_UP(params.h, 2) * 4); // Y16 + UV32 per 2px
        lineLength = params.w * 2; // 16bpp luma
    }
#endif
    else if (params.pixelFormat == PixelFormat::BGR16)
    {
        size = params.w * params.h * 2;
        lineLength = params.w * 2;
    }
    else if (params.pixelFormat == PixelFormat::RGB32 || params.pixelFormat == PixelFormat::BGR32)
    {
        size = params.w * params.h * 4;
        lineLength = params.w * 4;
    }

    if (size == 0)
    {
        Error(params.log, "Computed framebuffer size is 0 for linear layout");
        return false;
    }

    int ret = drmPrimeHandleToFD(params.deviceFd, params.framebuffer->handles[0], O_RDONLY, &fb_dmafd);
    if (ret != 0)
    {
        Error(params.log, "drmPrimeHandleToFD failed (handle=%u): %s", params.framebuffer->handles[0], strerror(errno));
        return false;
    }

    auto* mmapFrameBuffer = (uint8_t*)mmap(nullptr, size, PROT_READ, MAP_SHARED, fb_dmafd, 0);
    if (mmapFrameBuffer != MAP_FAILED)
    {
        params.imageResampler.processImage(mmapFrameBuffer, params.w, params.h, lineLength, params.pixelFormat, params.image);
        munmap(mmapFrameBuffer, size);
        close(fb_dmafd);
        return true;
    }

    Error(params.log, "Format: %s failed. Error: %s", QSTRING_CSTR(getDrmFormat(params.framebuffer->pixel_format)), strerror(errno));
    close(fb_dmafd);
    return false;
}

// --- Broadcom SAND helpers (format-agnostic dispatcher) ---

struct PlaneInfo
{
    int bpp;             // bits per pixel on this plane (8, 16, 32)
    int width;           // plane width in pixels
    int height;          // plane height in pixels
    int stride;          // destination packed bytes per line
    size_t srcBaseOffset; // source base offset (fb->offsets[idx])
    size_t dstBaseOffset; // destination base offset in packed buffer
    int fbPlaneIdx;      // FB plane index (0..3)
};

static bool getBroadcomSandGeometry(uint64_t modifier, uint32_t& columnWidthBytes, uint32_t& columnHeight, QString& errorString)
{
    switch (fourcc_mod_broadcom_mod(modifier))
    {
    case DRM_FORMAT_MOD_BROADCOM_SAND32:  columnWidthBytes = 32;  break;
    case DRM_FORMAT_MOD_BROADCOM_SAND64:  columnWidthBytes = 64;  break;
    case DRM_FORMAT_MOD_BROADCOM_SAND128: columnWidthBytes = 128; break;
    case DRM_FORMAT_MOD_BROADCOM_SAND256: columnWidthBytes = 256; break;
    case DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED:
        errorString = "Broadcom modifier 'DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED' currently not supported";
        return false;
    default:
        errorString = "Unknown Broadcom modifier";
        return false;
    }

    columnHeight = fourcc_mod_broadcom_param(modifier);
    return true;
}

static bool getPlanesForFormat(PixelFormat fmt,
                               int w, int h,
                               uint32_t /*columnWidthBytes*/,
                               const drmModeFB2* fb,
                               std::vector<PlaneInfo>& planes,
                               uint32_t& totalSize,
                               QString& errorString)
{
    planes.clear();
    totalSize = 0;

    switch (fmt)
    {
    case PixelFormat::NV12:
    {
        // Pack tightly: Y then interleaved UV
        auto const  y_stride = (uint32_t)w;
        auto const uv_stride = (uint32_t)w; // interleaved, one byte per pixel on luma grid

        PlaneInfo y{};
        y.bpp = 8;
        y.width = w;
        y.height = h;
        y.stride = (int)y_stride;
        y.fbPlaneIdx = 0;
        y.srcBaseOffset = fb->offsets[y.fbPlaneIdx];
        y.dstBaseOffset = 0;
        planes.push_back(y);

        PlaneInfo uv{};
        uv.bpp = 16; // two 8-bit chroma bytes per 2 luma samples horizontally
        uv.width = DIV_ROUND_UP(w, 2);
        uv.height = DIV_ROUND_UP(h, 2);
        uv.stride = (int)uv_stride;
        uv.fbPlaneIdx = 1;
        uv.srcBaseOffset = fb->offsets[uv.fbPlaneIdx];
        uv.dstBaseOffset = (size_t)w * (size_t)h;
        planes.push_back(uv);

        totalSize = (uint32_t)((w * h) + (w * h) / 2);
        return true;
    }

#ifdef DRM_FORMAT_NV21
    case PixelFormat::NV21:
    {
        // Identical plane sizes to NV12; chroma order is VU (handled by resampler)
        auto const y_stride = (uint32_t)w;
        auto const uv_stride = (uint32_t)w;

        PlaneInfo y{};
        y.bpp = 8;
        y.width = w;
        y.height = h;
        y.stride = (int)y_stride;
        y.fbPlaneIdx = 0;
        y.srcBaseOffset = fb->offsets[y.fbPlaneIdx];
        y.dstBaseOffset = 0;
        planes.push_back(y);

        PlaneInfo vu{};
        vu.bpp = 16;
        vu.width = DIV_ROUND_UP(w, 2);
        vu.height = DIV_ROUND_UP(h, 2);
        vu.stride = (int)uv_stride;
        vu.fbPlaneIdx = 1;
        vu.srcBaseOffset = fb->offsets[vu.fbPlaneIdx];
        vu.dstBaseOffset = (size_t)w * (size_t)h;
        planes.push_back(vu);

        totalSize = (uint32_t)((w * h) + (w * h) / 2);
        return true;
    }
#endif

#ifdef DRM_FORMAT_P030
    case PixelFormat::P030:
    {
        // 10-bit semi-planar: Y in 16-bit words, UV packed 2x10b per 2 luma pixels => 32b per pair
        auto const y_stride = (uint32_t)(w * 2);
        auto const uv_stride = (uint32_t)(w * 2); // (w/2)*4 bytes == 2*w bytes

        PlaneInfo y{};
        y.bpp = 16;
        y.width = w;
        y.height = h;
        y.stride = (int)y_stride;
        y.fbPlaneIdx = 0;
        y.srcBaseOffset = fb->offsets[y.fbPlaneIdx];
        y.dstBaseOffset = 0;
        planes.push_back(y);

        PlaneInfo uv{};
        uv.bpp = 32;
        uv.width = DIV_ROUND_UP(w, 2);
        uv.height = DIV_ROUND_UP(h, 2);
        uv.stride = (int)uv_stride;
        uv.fbPlaneIdx = 1;
        uv.srcBaseOffset = fb->offsets[uv.fbPlaneIdx];
        uv.dstBaseOffset = (size_t)w * (size_t)h * 2;
        planes.push_back(uv);

        totalSize = (uint32_t)((size_t)w * (size_t)h * 3); // Y(2*w*h) + UV(w*h)
        return true;
    }
#endif

    default:
        errorString = QString("Broadcom SAND: unsupported PixelFormat %1")
                          .arg(QSTRING_CSTR(pixelFormatToString(fmt)));
        return false;
    }
}

static inline void copyBroadcomSandPlane(const PlaneInfo& p,
                                         int lumaWidth,
                                         uint32_t columnWidthBytes,
                                         uint32_t columnHeight,
                                         const uint8_t* src,
                                         uint8_t* dst,
                                         Logger* log)
{
    const uint32_t columnSize = columnWidthBytes * columnHeight;
    // Keep proportional column width logic relative to luma width
    const size_t columnWidthInPixels = (size_t)(columnWidthBytes) * (size_t)p.width / std::max(1, lumaWidth);

    for (int i = 0; i < p.height; i++)
    {
        for (int j = 0; j < p.width; j++)
        {
            size_t src_offset = p.srcBaseOffset;
            size_t dst_offset = p.dstBaseOffset;

            src_offset += vc4_sand_tiled_offset(columnWidthInPixels, columnSize, (size_t)j, (size_t)i, (size_t)p.bpp);
            dst_offset += (size_t)p.stride * (size_t)i + (size_t)j * (size_t)p.bpp / 8u;

            switch (p.bpp)
            {
            case 8:
                *(dst + dst_offset) = *(src + src_offset);
                break;
            case 16:
                *(uint16_t*)(dst + dst_offset) = *(const uint16_t*)(src + src_offset);
                break;
            case 32:
                *(uint32_t*)(dst + dst_offset) = *(const uint32_t*)(src + src_offset);
                break;
            default:
                Error(log, "Unsupported bpp %d in Broadcom SAND layout", p.bpp);
                break;
            }
        }
    }
}

static bool untileBroadcomSandToLinear(int deviceFd,
                                       const drmModeFB2* fb,
                                       int w,
                                       int h,
                                       PixelFormat fmt,
                                       const std::vector<PlaneInfo>& planes,
                                       uint32_t totalSize,
                                       ImageResampler const& imageResampler,
                                       Logger* log,
                                       Image<ColorRgb>& image)
{
    int fb_dmafd = 0;
    int ret = drmPrimeHandleToFD(deviceFd, fb->handles[0], O_RDONLY, &fb_dmafd);
    if (ret < 0)
    {
        Error(log, "drmPrimeHandleToFD failed (broadcom handle=%u): %s", fb->handles[0], strerror(errno));
        return false;
    }

    if (totalSize == 0)
    {
        Error(log, "Computed framebuffer size is 0 for Broadcom SAND layout");
        close(fb_dmafd);
        return false;
    }

    auto* src_buf = (uint8_t*)mmap(nullptr, totalSize, PROT_READ, MAP_SHARED, fb_dmafd, 0);
    if (src_buf == MAP_FAILED)
    {
        close(fb_dmafd);
        return false;
    }

    std::vector<uint8_t> dst_buf(totalSize);
    uint8_t* dst_ptr = dst_buf.data();

    uint32_t columnWidthBytes = 0;
    uint32_t columnHeight = 0;
    QString geomErr;
    if (!getBroadcomSandGeometry(fb->modifier, columnWidthBytes, columnHeight, geomErr))
    {
        munmap(src_buf, totalSize);
        close(fb_dmafd);
        return false;
    }

    for (const auto& p : planes)
    {
        copyBroadcomSandPlane(p, w, columnWidthBytes, columnHeight, src_buf, dst_ptr, log);
    }

    int lineLength = w;
#ifdef DRM_FORMAT_P030
    if (fmt == PixelFormat::P030)
        lineLength = w * 2;
#endif

    imageResampler.processImage(dst_ptr, w, h, lineLength, fmt, image);

    munmap(src_buf, totalSize);
    close(fb_dmafd);
    return true;
}

static bool processBroadcomSandFramebuffer(int deviceFd,
                                           const drmModeFB2* framebuffer,
                                           int w,
                                           int h,
                                           PixelFormat pixelFormat,
                                           ImageResampler const& imageResampler,
                                           Logger* log,
                                           Image<ColorRgb>& image,
                                           QString& errorString)
{
    uint32_t columnWidthBytes = 0;
    uint32_t columnHeight = 0;
    if (!getBroadcomSandGeometry(framebuffer->modifier, columnWidthBytes, columnHeight, errorString))
    {
        return false;
    }

    std::vector<PlaneInfo> planes;
    uint32_t totalSize = 0;
    if (!getPlanesForFormat(pixelFormat, w, h, columnWidthBytes, framebuffer, planes, totalSize, errorString))
    {
        return false;
    }

    if (totalSize == 0)
    {
        Error(log, "Computed framebuffer size is 0 for Broadcom SAND layout");
        return false;
    }

    return untileBroadcomSandToLinear(deviceFd, framebuffer, w, h, pixelFormat, planes, totalSize, imageResampler, log, image);
}

int DRMFrameGrabber::grabFrame(Image<ColorRgb> &image)
{
    if (!_isEnabled || _isDeviceInError)
    {
        return -1;
    }

    if (_framebuffers.empty())
    {
        Error(_log, "No framebuffers found. Was setupScreen() successful?");
        return -1;
    }

    bool newImage{false};
    QString errorString;

    // We only need to process the first framebuffer.
    auto it = _framebuffers.begin();
    if (it != _framebuffers.end())
    {
        const auto& [id, framebuffer] = *it;

        _pixelFormat = GetPixelFormatForDrmFormat(framebuffer->pixel_format);
        uint64_t modifier = framebuffer->modifier;

        qCDebug(grabber_screen_capture) << QString("Framebuffer ID: %1 - Width: %2 - Height: %3  - DRM Format: %4 - PixelFormat: %5, Modifier: %6")
                .arg(id)
                .arg(framebuffer->width)
                .arg(framebuffer->height)
                .arg(getDrmFormat(framebuffer->pixel_format))
                .arg(pixelFormatToString(_pixelFormat))
                .arg(getDrmModifierName(modifier));

        int w = framebuffer->width;
        int h = framebuffer->height;
        Grabber::setWidthHeight(w, h);

        // Linear modifier path
        if (_pixelFormat != PixelFormat::NO_CHANGE && modifier == DRM_FORMAT_MOD_LINEAR)
        {
            LinearFramebufferParams params{
                _deviceFd,
                framebuffer,
                w,
                h,
                _pixelFormat,
                _imageResampler,
                _log,
                image
            };
            if (processLinearFramebuffer(params))
            {
                newImage = true;
            }
        }
        // Broadcom SAND path
        else if ((modifier >> 56ULL) == DRM_FORMAT_MOD_VENDOR_BROADCOM)
        {
            if (processBroadcomSandFramebuffer(_deviceFd, framebuffer, w, h, _pixelFormat, _imageResampler, _log.data(), image, errorString))
            {
                newImage = true;
            }
        }
        else
        {
            errorString = QString("Currently unsupported format: %1 and/or modifier: %2")
                              .arg(getDrmFormat(framebuffer->pixel_format))
                              .arg(getDrmModifierName(framebuffer->modifier));
        }
    }

    if (!errorString.isEmpty())
    {
        this->setInError(errorString);
        return -1;
    }

    if (!newImage)
    {
        qCDebug(grabber_screen_capture) << "No image captured from DRM framebuffer.";
        return -1;
    }

    return 0;
}

bool DRMFrameGrabber::openDevice()
{
    if (_deviceFd >= 0)
    {
        return true;
    }

    if (!_isAvailable)
    {
        return false;
    }

    // Try read-only first to minimize required privileges. Some drivers require O_RDWR; fallback in that case.
    _deviceFd = ::open(QSTRING_CSTR(getDeviceName()), O_RDONLY | O_CLOEXEC);
    if (_deviceFd < 0)
    {
        // fallback to read-write if required by driver
        _deviceFd = ::open(QSTRING_CSTR(getDeviceName()), O_RDWR | O_CLOEXEC);
    }
    if (_deviceFd < 0)
    {
        QString errorReason = QString("Error opening %1, [%2] %3").arg(getDeviceName()).arg(errno).arg(std::strerror(errno));
        this->setInError(errorReason);
        return false;
    }

    return true;
}

bool DRMFrameGrabber::closeDevice()
{
    if (_deviceFd < 0)
    {
        return true;
    }

    bool success = (::close(_deviceFd) == 0);
    _deviceFd = -1;

    return success;
}

QSize DRMFrameGrabber::getScreenSize() const
{
    return getScreenSize(getDeviceName());
}

inline QSize findActiveCrtcSize(int drmfd, const drmModeConnector* connector)
{
    if (!connector || !connector->encoder_id)
    {
        return QSize();
    }

    drmModeEncoderPtr encoder = drmModeGetEncoder(drmfd, connector->encoder_id);
    if (!encoder)
    {
        return QSize();
    }

    QSize size;
    if (encoder->crtc_id)
    {
        drmModeCrtcPtr crtc = drmModeGetCrtc(drmfd, encoder->crtc_id);
        if (crtc)
        {
            size.setWidth(crtc->width);
            size.setHeight(crtc->height);
            drmModeFreeCrtc(crtc);
        }
    }

    drmModeFreeEncoder(encoder);
    return size;
}

QSize DRMFrameGrabber::getScreenSize(const QString &device) const
{
    DrmResources drmResources;
    if (!discoverDrmResources(device, drmResources))
    {
        return {};
    }

    // 3. Iterate through connectors to find a connected one
    for (const auto& connector : drmResources.connectors)
    {
        if (connector->connection != DRM_MODE_CONNECTED || connector->count_modes <= 0)
        {
            continue;
        }

        for (const auto& crtc : drmResources.crtcs)
        {
            if (crtc->mode_valid)
            {
                return QSize(crtc->width, crtc->height);
            }
        }
    }

    return {};
}

bool DRMFrameGrabber::discoverDrmResources(const QString& device, DrmResources& resources) const
{
	// 1. Open the DRM device
	int drmfd = ::open(QSTRING_CSTR(device), O_RDWR);
	if (drmfd < 0)
	{
		return false;
	}

	// 2. Get device resources
	drmModeResPtr drmModeResources = drmModeGetResources(drmfd);
	if (!drmModeResources)
	{
		::close(drmfd);
		return false;
	}

	for (int i = 0; i < drmModeResources->count_connectors; ++i)
	{
		drmModeConnectorPtr connector = drmModeGetConnector(drmfd, drmModeResources->connectors[i]);
		if (connector)
		{
			resources.connectors.emplace_back(connector, drmModeFreeConnector);
		}
	}

	for (int i = 0; i < drmModeResources->count_crtcs; ++i)
	{
		drmModeCrtcPtr crtc = drmModeGetCrtc(drmfd, drmModeResources->crtcs[i]);
		if (crtc)
		{
			resources.crtcs.emplace_back(crtc, drmModeFreeCrtc);
		}
	}

	drmModeFreeResources(drmModeResources);
	::close(drmfd);

	return true;
}

void DRMFrameGrabber::setDrmClientCaps()
{
	if (drmSetClientCap(_deviceFd, DRM_CLIENT_CAP_ATOMIC, 1) < 0)
	{
		Debug(_log, "drmSetClientCap(DRM_CLIENT_CAP_ATOMIC) failed: %s", strerror(errno));
	}
	if (drmSetClientCap(_deviceFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) < 0)
	{
		Debug(_log, "drmSetClientCap(DRM_CLIENT_CAP_UNIVERSAL_PLANES) failed: %s", strerror(errno));
	}
}

void DRMFrameGrabber::enumerateConnectorsAndEncoders(const drmModeRes* resources)
{
	for (int i = 0; i < resources->count_connectors; i++)
	{
		auto connector = std::make_unique<Connector>();
		connector->ptr = drmModeGetConnector(_deviceFd, resources->connectors[i]);
		_connectors.insert_or_assign(resources->connectors[i], std::move(connector));
	}

	for (int i = 0; i < resources->count_encoders; i++)
	{
		auto encoder = std::make_unique<Encoder>();
		encoder->ptr = drmModeGetEncoder(_deviceFd, resources->encoders[i]);
		_encoders.insert_or_assign(resources->encoders[i], std::move(encoder));
	}
}

void DRMFrameGrabber::findActiveCrtc(const drmModeRes* resources)
{
    for (int i = 0; i < resources->count_crtcs; i++)
    {
        _crtc = drmModeGetCrtc(_deviceFd, resources->crtcs[i]);
        if (_crtc && _crtc->mode_valid)
        {
            return; // Found active CRTC, so we can exit
        }
        drmModeFreeCrtc(_crtc);
        _crtc = nullptr;
    }
}

bool DRMFrameGrabber::isPrimaryPlaneForCrtc(uint32_t planeId, const drmModeObjectProperties* properties)
{
    for (unsigned int j = 0; j < properties->count_props; ++j)
    {
        auto prop = drmModeGetProperty(_deviceFd, properties->props[j]);
        if (!prop) continue;

        bool isPrimary = (strcmp(prop->name, "type") == 0 && properties->prop_values[j] == DRM_PLANE_TYPE_PRIMARY);
        drmModeFreeProperty(prop);

        if (isPrimary)
        {
            auto plane = drmModeGetPlane(_deviceFd, planeId);
            if (plane && _crtc && plane->crtc_id == _crtc->crtc_id)
            {
                qCDebug(grabber_screen_flow) << plane;
                _planes.insert({planeId, plane});
                return true;
            }
            if (plane)
            {
                drmModeFreePlane(plane);
            }
        }
    }
    return false;
}

void DRMFrameGrabber::findPrimaryPlane(const drmModePlaneRes* planeResources)
{
    for (unsigned int i = 0; i < planeResources->count_planes; ++i)
    {
        uint32_t planeId = planeResources->planes[i];
		auto properties = drmModeObjectGetProperties(_deviceFd, planeId, DRM_MODE_OBJECT_PLANE);
		if (!properties) continue;

		bool found = isPrimaryPlaneForCrtc(planeId, properties);
		drmModeFreeObjectProperties(properties);

		if (found)
		{
			break;
		}
	}
}

void DRMFrameGrabber::getDrmObjectProperties() const
{
	for (auto const &[id, connector] : _connectors)
	{
		auto properties = drmModeObjectGetProperties(_deviceFd, id, DRM_MODE_OBJECT_CONNECTOR);
		if (!properties) continue;
		for (unsigned int i = 0; i < properties->count_props; i++)
		{
			auto prop = drmModeGetProperty(_deviceFd, properties->props[i]);
			connector->props.insert({std::string(prop->name), {.spec = prop, .value = properties->prop_values[i]}});
		}
		drmModeFreeObjectProperties(properties);
	}

	for (auto const &[id, encoder] : _encoders)
	{
		auto properties = drmModeObjectGetProperties(_deviceFd, id, DRM_MODE_OBJECT_ENCODER);
		if (!properties) continue;
		for (unsigned int i = 0; i < properties->count_props; i++)
		{
			auto prop = drmModeGetProperty(_deviceFd, properties->props[i]);
			encoder->props.insert({std::string(prop->name), {.spec = prop, .value = properties->prop_values[i]}});
		}
		drmModeFreeObjectProperties(properties);
	}
}

void DRMFrameGrabber::getFramebuffers()
{
	for (auto const &[id, plane] : _planes)
	{
		drmModeFB2Ptr fb = drmModeGetFB2(_deviceFd, plane->fb_id);
		qCDebug(grabber_screen_flow) << fb;
		if (fb == nullptr) continue;

		if (fb->handles[0] == 0)
		{
			setInError("Not able to acquire framebuffer handles. Screen capture not possible. Check permissions.");
			drmModeFreeFB2(fb);
			continue;
		}
		_framebuffers.insert({plane->fb_id, fb});
	}
}

bool DRMFrameGrabber::getScreenInfo()
{
	if (_deviceFd < 0)
	{
		this->setInError("DRM device not open");
		return false;
	}

	setDrmClientCaps();

	drmModeResPtr resources = drmModeGetResources(_deviceFd);
	if (!resources)
	{
		this->setInError(QString("Unable to get DRM resources on %1").arg(getDeviceName()));
		return false;
	}

	enumerateConnectorsAndEncoders(resources);
	findActiveCrtc(resources);

	drmModePlaneResPtr planeResources = drmModeGetPlaneResources(_deviceFd);
	if (planeResources)
	{
		findPrimaryPlane(planeResources);
		drmModeFreePlaneResources(planeResources);
	}
	else
	{
		Debug(_log, "drmModeGetPlaneResources returned NULL or failed: %s", strerror(errno));
	}

	getDrmObjectProperties();
	getFramebuffers();

	drmModeFreeResources(resources);

	qCDebug(grabber_screen_flow) << "Framebuffer count:" << _framebuffers.size();

	return !_framebuffers.empty();
}

QJsonArray DRMFrameGrabber::getInputDeviceDetails() const
{
	// Find framebuffer devices 0-9
	QDir deviceDirectory(DRM_DIR_NAME);
	QStringList deviceFilter(QString("%1%2").arg(DRM_PRIMARY_MINOR_NAME).arg('?'));
	deviceDirectory.setNameFilters(deviceFilter);
	deviceDirectory.setSorting(QDir::Name);
	QFileInfoList deviceFiles = deviceDirectory.entryInfoList(QDir::System);

	QJsonArray video_inputs;
	for (const auto &deviceFile : deviceFiles)
	{
		QString const fileName = deviceFile.fileName();
		int deviceIdx = fileName.right(1).toInt();
		QString device = deviceFile.absoluteFilePath();
		qCDebug(grabber_screen_properties) << "DRM device [" << device << "] found";

		QSize screenSize = getScreenSize(device);
		//Only add devices with a valid screen size, i.e. where a monitor is connected
		if ( !screenSize.isEmpty() )
		{
			QJsonArray fps = {"1", "5", "10", "15", "20", "25", "30", "40", "50", "60"};

			QJsonObject input;

			QString displayName;
			displayName = QString("Output %1").arg(deviceIdx);

			input["name"] = displayName;
			input["inputIdx"] = deviceIdx;

			QJsonArray formats;
			QJsonObject format;

			QJsonArray resolutionArray;

			QJsonObject resolution;

			resolution["width"] = screenSize.width();
			resolution["height"] = screenSize.height();
			resolution["fps"] = fps;

			resolutionArray.append(resolution);

			format["resolutions"] = resolutionArray;
			formats.append(format);

			input["formats"] = formats;
			video_inputs.append(input);
		}
	}

	return video_inputs;
}

QJsonObject DRMFrameGrabber::discover(const QJsonObject & /*params*/)
{
	if (!isAvailable(false))
	{
		return {};
	}

	QJsonObject inputsDiscovered;

	QJsonArray const video_inputs = getInputDeviceDetails();
	if (video_inputs.isEmpty())
	{
		qCDebug(grabber_screen_properties) << "No displays found to capture from!";
		return {};
	}

	inputsDiscovered["device"] = "drm";
	inputsDiscovered["device_name"] = "DRM";
	inputsDiscovered["type"] = "screen";
	inputsDiscovered["video_inputs"] = video_inputs;

	QJsonObject defaults;
	QJsonObject video_inputs_default;
	QJsonObject resolution_default;

	resolution_default["fps"] = _fps;
	video_inputs_default["resolution"] = resolution_default;
	video_inputs_default["inputIdx"] = 0;
	defaults["video_input"] = video_inputs_default;
	inputsDiscovered["default"] = defaults;

	return inputsDiscovered;
}

QDebug operator<<(QDebug dbg, const drmModeFB2* fb)
{
	QDebugStateSaver saver(dbg);
	dbg.nospace();

	if (!fb)
	{
		dbg << "drmModeFB2Ptr(null)";
		return dbg;
	}

	dbg << "drmModeFB2("
		<< "id: " << fb->fb_id
		<< ", size: " << fb->width << "x" << fb->height
		<< ", DRM format: " << getDrmFormat(fb->pixel_format)
		<< ", DRM modifier: " << getDrmModifierName(fb->modifier)
		<< ", flags: " << Qt::hex << fb->flags << Qt::dec
		<< ", handles: " << fb->handles[0] << fb->handles[1] << fb->handles[2] << fb->handles[3]
		<< ", pitches: " << fb->pitches[0] << fb->pitches[1] << fb->pitches[2] << fb->pitches[3]
		<< ", offsets: " << fb->offsets[0] << fb->offsets[1] << fb->offsets[2] << fb->offsets[3]
		<< ")";

	return dbg;
}

QDebug operator<<(QDebug dbg, const drmModePlane* plane)
{
	QDebugStateSaver saver(dbg);
	dbg.nospace();

	if (!plane)
	{
		dbg << "drmModePlanePtr(null)";
		return dbg;
	}

	dbg << "drmModePlane("
		<< "id: " << plane->plane_id
		<< ", CRTC id: " << plane->crtc_id
		<< ", FB id: " << plane->fb_id
		<< ", pos: " << plane->x << "," << plane->y
		<< ", CRTC pos: " << plane->crtc_x << "," << plane->crtc_y
		<< ", possible CRTCs: " << Qt::hex << plane->possible_crtcs << Qt::dec
		<< ", gamma size: " << plane->gamma_size
		<< ", format count: " << plane->count_formats
		<< ")";

	return dbg;
}