define(['lib/stapes', 'api/Socket', 'utils/Tools'], function (Stapes, Socket, tools) {
    'use strict';
    return Socket.subclass(/** @lends WebSocket.prototype  */{
        DEBUG: false,

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
         * @class WebSocket
         * @extends Socket
         * @constructs
         */
        constructor: function () {
            this.inputBuffer = new Uint8Array(4096);
            this.inputBufferIndex = 0;
        },

        create: function (onSuccess, onError) {
            if (this.DEBUG) {
                console.log('[DEBUG] Creating socket...');
            }

            if (onSuccess) {
                onSuccess();
            }
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

            if (resultCallback) {
                if (this.handle.readyState === WebSocket.OPEN) {
                    resultCallback(true);
                } else {
                    resultCallback(false);
                }
            }
        },

        connect: function (server, onSuccess, onError) {
            if (this.DEBUG) {
                console.log('[DEBUG] Connecting to peer ' + server.address + ':' + server.port);
            }

            this.currentErrorCallback = onError;

            this.handle = new WebSocket('ws://' + server.address + ':' + server.port);
            this.handle.onmessage = this.onDataReceived.bind(this);
            this.handle.onclose = function () {
                if (this.DEBUG) {
                    console.log('onClose');
                }
            }.bind(this);
            this.handle.onerror = function () {
                if (this.DEBUG) {
                    console.log('[ERROR]: ');
                }

                if (this.currentErrorCallback) {
                    this.currentErrorCallback('WebSocket error');
                    this.currentErrorCallback = null;
                }
            }.bind(this);
            this.handle.onopen = function () {
                if (onSuccess) {
                    onSuccess();
                }
            };
        },

        close: function (onSuccess, onError) {
            if (this.DEBUG) {
                console.log('[DEBUG] Closing socket...');
            }

            if (this.handle) {
                this.handle.close();
                if (onSuccess) {
                    onSuccess();
                }
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
                        //dataToSend = tools.str2ab(data);
                        dataToSend = data;
                    } else {
                        if (this.DEBUG) {
                            console.log('> ' + tools.ab2hexstr(data));
                        }
                        dataToSend = data;
                    }

                    this.currentErrorCallback = onError;
                    this.handle.send(dataToSend);

                    if (onSuccess) {
                        onSuccess();
                    }
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
         * @param event
         */
        onDataReceived: function (event) {
            if (this.DEBUG) {
                console.log('[DEBUG] received data...');
            }

            if (this.handle && event.data) {
                if (this.DEBUG) {
                    console.log('< ' + event.data);
                }

                if (this.currentResponseCallback) {
                    this.currentResponseCallback(tools.str2ab(event.data));
                    this.currentResponseCallback = null;
                }
            }
        }
    }, true);
});
