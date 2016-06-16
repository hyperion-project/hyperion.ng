#pragma once

#include <QObject>

#ifdef ENABLE_DISPMANX
	#include <grabber/DispmanxWrapper.h>
#else
	typedef QObject DispmanxWrapper;
#endif

#ifdef ENABLE_V4L2
	#include <grabber/V4L2Wrapper.h>
#else
	typedef QObject V4L2Wrapper;
#endif

#ifdef ENABLE_FB
	#include <grabber/FramebufferWrapper.h>
#else
	typedef QObject FramebufferWrapper;
#endif

#ifdef ENABLE_AMLOGIC
	#include <grabber/AmlogicWrapper.h>
#else
	typedef QObject AmlogicWrapper;
#endif

#ifdef ENABLE_OSX
	#include <grabber/OsxWrapper.h>
#else
	typedef QObject OsxWrapper;
#endif

#include <xbmcvideochecker/XBMCVideoChecker.h>
#include <jsonserver/JsonServer.h>
#include <protoserver/ProtoServer.h>
#include <boblightserver/BoblightServer.h>

void startBootsequence();
XBMCVideoChecker* createXBMCVideoChecker();
void startNetworkServices(JsonServer* &jsonServer, ProtoServer* &protoServer, BoblightServer* &boblightServer);

// grabber creators
DispmanxWrapper*    createGrabberDispmanx(ProtoServer* &protoServer);
V4L2Wrapper*        createGrabberV4L2(ProtoServer* &protoServer );
AmlogicWrapper*     createGrabberAmlogic(ProtoServer* &protoServer);
FramebufferWrapper* createGrabberFramebuffer(ProtoServer* &protoServer);
OsxWrapper*         createGrabberOsx(ProtoServer* &protoServer);
