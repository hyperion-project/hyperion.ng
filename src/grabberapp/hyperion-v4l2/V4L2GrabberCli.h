#pragma once

#include <QCoreApplication>

#include "V4L2GrabberOptions.h"

V4L2GrabberOptions parseV4L2GrabberOptions(const QCoreApplication& app);
