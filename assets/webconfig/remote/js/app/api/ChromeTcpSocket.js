/*global chrome */
define(['lib/stapes', 'api/Socket', 'utils/Tools'], function (Stapes, Socket, tools) {
    'use strict';
    return Socket.subclass(/** @lends ChromeTcpSocket.prototype  */{
        DEBUG: false,

        /**
         * @type {number}
         */
        handle: null,

        /**
         * @type {function}
         */
        currentResponseCallback: null,

        /**
         * @type {function}
         */
        currentErrorCallback: null,

        /**
         * Temporary buffer for incoming data
         * @type {Uint8Array}
         */
        inputBuffer: null,
        inputBufferIndex: 0,
        readBufferTimerId: null,

        /**
         * @class ChromeTcpSocket
         * @extends Socket
         * @constructs
         */
        constructor: function () {
            this.inputBuffer = new Uint8Array(4096);
            this.inputBufferIndex = 0;

            chrome.sockets.tcp.onReceive.addListener(this.onDataReceived.bind(this));
            chrome.sockets.tcp.onReceiveError.addListener(this.onError.bind(this));
        },

        create: function (onSuccess, onError) {
            if (this.DEBUG) {
                console.log('[DEBUG] Creating socket...');
            }
            chrome.sockets.tcp.create({bufferSize: 4096}, function (createInfo) {
                if (this.DEBUG) {
                    console.log('[DEBUG] Socket created: ' + createInfo.socketId);
                }
                this.handle = createInfo.socketId;
                if (onSuccess) {
                    onSuccess();
                }
            }.bind(this));
        },

        isConnected: function (resultCallback) {
            if (this.DEBUG) {
                console.log('[DEBUG] Checking if socket is connected...');
            }

            if (!this.handle) {
                if (this.DEBUG) {
                    console.log('[DEBUG] Socket not created');
                }

                if (resultCallback) {
                    resultCallback(false);
                }
                return;
            }

            chrome.sockets.tcp.getInfo(this.handle, function (socketInfo) {
                if (this.DEBUG) {
                    console.log('[DEBUG] Socket connected: ' + socketInfo.connected);
                }

                if (socketInfo.connected) {
                    if (resultCallback) {
                        resultCallback(true);
                    }
                } else {
                    if (resultCallback) {
                        resultCallback(false);
                    }
                }
            }.bind(this));
        },

        connect: function (server, onSuccess, onError) {
            var timeoutHandle;

            if (this.DEBUG) {
                console.log('[DEBUG] Connecting to peer ' + server.address + ':' + server.port);
            }

            if (!this.handle) {
                if (this.DEBUG) {
                    console.log('[DEBUG] Socket not created');
                }

                if (onError) {
                    onError('Socket handle is invalid');
                }
                return;
            }

            // FIXME for some reason chrome blocks if peer is not reachable
            timeoutHandle = setTimeout(function () {
                chrome.sockets.tcp.getInfo(this.handle, function (socketInfo) {
                    if (!socketInfo.connected) {
                        // let the consumer decide if to close or not?
                        // this.close();
                        onError('Could not connect to ' + server.address + ':' + server.port);
                    }
                }.bind(this));
            }.bind(this), 500);

            chrome.sockets.tcp.connect(this.handle, server.address, server.port, function (result) {
                if (this.DEBUG) {
                    console.log('[DEBUG] Connect result: ' + result);
                }
                clearTimeout(timeoutHandle);

                if (chrome.runtime.lastError) {
                    if (onError) {
                        onError('Could not connect to ' + server.address + ':' + server.port);
                    }
                    return;
                }

                if (result !== 0) {
                    if (onError) {
                        onError('Could not connect to ' + server.address + ':' + server.port);
                    }
                } else if (onSuccess) {
                    onSuccess();
                }
            }.bind(this));
        },

        close: function (onSuccess, onError) {
            if (this.DEBUG) {
                console.log('[DEBUG] Closing socket...');
            }

            if (this.handle) {
                chrome.sockets.tcp.close(this.handle, function () {
                    this.handle = null;
                    if (onSuccess) {
                        onSuccess();
                    }
                }.bind(this));
            } else {
                if (this.DEBUG) {
                    console.log('[DEBUG] Socket not created');
                }

                if (onError) {
                    onError('Socket handle is invalid');
                }
            }
        },

        write: function (data, onSuccess, onError) {
            var dataToSend = null, dataType = typeof (data);

            if (this.DEBUG) {
                console.log('[DEBUG] writing to socket...');
            }

            if (!this.handle) {
                if (this.DEBUG) {
                    console.log('[DEBUG] Socket not created');
                }

                if (onError) {
                    onError('Socket handle is invalid');
                }
                return;
            }

            this.isConnected(function (connected) {
                if (connected) {
                    if (dataType === 'string') {
                        if (this.DEBUG) {
                            console.log('> ' + data);
                        }
                        dataToSend = tools.str2ab(data);
                    } else {
                        if (this.DEBUG) {
                            console.log('> ' + tools.ab2hexstr(data));
                        }
                        dataToSend = data;
                    }

                    chrome.sockets.tcp.send(this.handle, tools.a2ab(dataToSend), function (sendInfo) {
                        if (this.DEBUG) {
                            console.log('[DEBUG] Socket write result: ' + sendInfo.resultCode);
                        }

                        if (sendInfo.resultCode !== 0) {
                            onError('Socket write error: ' + sendInfo.resultCode);
                        } else if (onSuccess) {
                            onSuccess();
                        }
                    }.bind(this));
                } else {
                    if (onError) {
                        onError('No connection to peer');
                    }
                }
            }.bind(this));

        },

        read: function (onSuccess, onError) {
            if (this.DEBUG) {
                console.log('[DEBUG] reading from socket...');
            }

            if (!this.handle) {
                if (this.DEBUG) {
                    console.log('[DEBUG] socket not created');
                }

                if (onError) {
                    onError('Socket handle is invalid');
                }
                return;
            }

            this.isConnected(function (connected) {
                if (!connected) {
                    this.currentResponseCallback = null;
                    this.currentErrorCallback = null;

                    if (onError) {
                        onError('No connection to peer');
                    }
                }
            }.bind(this));

            if (onSuccess) {
                this.currentResponseCallback = onSuccess;
            }

            if (onError) {
                this.currentErrorCallback = onError;
            }
        },

        /**
         * Data receiption callback
         * @private
         * @param info
         */
        onDataReceived: function (info) {
            if (this.DEBUG) {
                console.log('[DEBUG] received data...');
            }

            if (info.socketId === this.handle && info.data) {
                if (this.readBufferTimerId) {
                    clearTimeout(this.readBufferTimerId);
                }
                if (this.readTimeoutTimerId) {
                    clearTimeout(this.readTimeoutTimerId);
                    this.readTimeoutTimerId = null;
                }
                this.inputBuffer.set(new Uint8Array(info.data), this.inputBufferIndex);
                this.inputBufferIndex += info.data.byteLength;

                if (this.DEBUG) {
                    console.log('< ' + tools.ab2hexstr(info.data));
                }

                if (this.currentResponseCallback) {
                    this.readBufferTimerId = setTimeout(function () {
                        this.currentResponseCallback(this.inputBuffer.subarray(0, this.inputBufferIndex));
                        this.inputBufferIndex = 0;
                        this.currentResponseCallback = null;
                    }.bind(this), 200);
                }
            }
        },

        /**
         * Error callback
         * @private
         * @param info
         */
        onError: function (info) {
            if (this.DEBUG) {
                console.log('[ERROR]: ' + info.resultCode);
            }

            if (info.socketId === this.handle) {
                if (this.currentErrorCallback) {
                    this.currentErrorCallback(info.resultCode);
                    this.currentErrorCallback = null;
                }
            }
        }
    }, true);
});
