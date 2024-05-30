#ifndef GRABBERCONFIG_H
#define GRABBERCONFIG_H

#if defined(ENABLE_MF)
#include <grabber/video/mediafoundation/MFGrabber.h>
#elif defined(ENABLE_V4L2)
#include <grabber/video/v4l2/V4L2Grabber.h>
#endif

#if defined(ENABLE_AUDIO)
#include <grabber/audio/AudioGrabber.h>

#ifdef WIN32
#include <grabber/audio/AudioGrabberWindows.h>
#endif

#ifdef __linux__
#include <grabber/audio/AudioGrabberLinux.h>
#endif
#endif

#ifdef ENABLE_QT
#include <grabber/qt/QtGrabber.h>
#endif

#ifdef ENABLE_DX
#include <grabber/directx/directXGrabber.h>
#endif

#ifdef ENABLE_DDA
#include <grabber/dda/DDAGrabber.h>
#endif

#if defined(ENABLE_X11)
#include <grabber/x11/X11Grabber.h>
#endif

#if defined(ENABLE_XCB)
#include <grabber/xcb/XcbGrabber.h>
#endif

#if defined(ENABLE_FB)
#include <grabber/framebuffer/FramebufferFrameGrabber.h>
#endif

#if defined(ENABLE_DISPMANX)
#include <grabber/dispmanx/DispmanxFrameGrabber.h>
#endif

#if defined(ENABLE_AMLOGIC)
#include <grabber/amlogic/AmlogicGrabber.h>
#endif

#if defined(ENABLE_OSX)
#include <grabber/osx/OsxFrameGrabber.h>
#endif

#endif // GRABBERCONFIG_H
