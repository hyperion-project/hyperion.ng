#pragma once

// parent class
#include <api/API.h>

class ProtoAPI : public API
{
    Q_OBJECT

public:
    ///
    /// Constructor
    ///
    /// @param peerAddress provide the Address of the peer
    /// @param log         The Logger class of the creator
    /// @param parent      Parent QObject
    /// @param localConnection True when the sender has origin home network
    ///
    ProtoAPI(QString peerAddress, Logger *log, const bool &localConnection, QObject *parent);

    ///
    /// @brief Initialization steps
    ///
    void initialize(void);
};