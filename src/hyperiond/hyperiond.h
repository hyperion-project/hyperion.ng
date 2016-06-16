// config includes
#include "HyperionConfig.h"

#include <QObject>
// Json-Schema includes
#include <utils/jsonschema/JsonFactory.h>

#ifdef ENABLE_DISPMANX
// Dispmanx grabber includes
#include <grabber/DispmanxWrapper.h>
#else
typedef QObject DispmanxWrapper;
#endif

#ifdef ENABLE_V4L2
// v4l2 grabber
#include <grabber/V4L2Wrapper.h>
#else
typedef QObject V4L2Wrapper;
#endif

#ifdef ENABLE_FB
// Framebuffer grabber includes
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
// OSX grabber includes
#include <grabber/OsxWrapper.h>
#else
typedef QObject OsxWrapper;
#endif

// XBMC Video checker includes
#include <xbmcvideochecker/XBMCVideoChecker.h>


// network servers
#include <jsonserver/JsonServer.h>
#include <protoserver/ProtoServer.h>
#include <boblightserver/BoblightServer.h>
#include <utils/Logger.h>

void signal_handler(const int signum);
Json::Value loadConfig(const std::string & configFile);
void startNewHyperion(int parentPid, std::string hyperionFile, std::string configFile);
void startBootsequence();
XBMCVideoChecker* createXBMCVideoChecker();
void startNetworkServices(JsonServer* &jsonServer, ProtoServer* &protoServer, BoblightServer* &boblightServer);

DispmanxWrapper* createGrabberDispmanx(ProtoServer* &protoServer);
V4L2Wrapper* createGrabberV4L2(ProtoServer* &protoServer );
AmlogicWrapper* createGrabberAmlogic(ProtoServer* &protoServer);
FramebufferWrapper* createGrabberFramebuffer(ProtoServer* &protoServer);
OsxWrapper* createGrabberOsx(ProtoServer* &protoServer);
