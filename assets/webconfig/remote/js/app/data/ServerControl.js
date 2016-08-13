/*global define */
define(['lib/stapes', 'utils/Tools'], function (Stapes, tools) {
    'use strict';

    return Stapes.subclass(/** @lends ServerControl.prototype */{
        /** @type Socket */
        socket: null,
        server: null,
        connecting: false,

        /**
         * @class ServerControl
         * @classdesc Interface for the hyperion server control. All commands are sent directly to hyperion's server.
         * @constructs
         * @param {object} server - Hyperion server parameter
         * @param {string} server.address - Server address
         * @param {number} server.port - Hyperion server port
         * @param {function} Socket - constructor of the socket to use for communication
         *
         * @fires connected
         * @fires error
         * @fires serverInfo
         * @fires cmdSent
         */
        constructor: function (server, Socket) {
            this.server = server;
            this.socket = new Socket();
            this.connecting = false;
        },

        /**
         * Try to connect to the server
         */
        connect: function () {
            if (!this.server) {
                this.emit('error', 'Missing server info');
            } else {
                this.connecting = true;
                this.socket.create(function () {
                    this.socket.connect(this.server, function () {
                        this.emit('connected');
                        this.connecting = false;
                    }.bind(this), function (error) {
                        this.socket.close();
                        this.emit('error', error);
                        this.connecting = false;
                    }.bind(this));
                }.bind(this));
            }
        },

        /**
         * Disconnect from the server
         */
        disconnect: function () {
            this.socket.close();
        },

        /**
         * Sends the color command to the server
         * @param {object} color - Color to set
         * @param {number} color.r - Red value
         * @param {number} color.g - Green value
         * @param {number} color.b - Blue value
         * @param {number} duration - Duration in seconds
         */
        setColor: function (color, duration) {
            var intColor, cmd;

            intColor = [
                Math.floor(color.r), Math.floor(color.g), Math.floor(color.b)
            ];
            cmd = {
                command: 'color',
                color: intColor,
                priority: this.server.priority
            };

            if (duration) {
                cmd.duration = duration * 1000;
            }

            this.sendCommand(cmd);
        },

        clear: function () {
            var cmd = {
                command: 'clear',
                priority: this.server.priority
            };
            this.sendCommand(cmd);
        },

        clearall: function () {
            var cmd = {
                command: 'clearall'
            };
            this.sendCommand(cmd);
        },

        /**
         * Sends a command to rund specified effect
         * @param {object} effect - Effect object
         */
        runEffect: function (effect) {
            var cmd;

            if (!effect) {
                return;
            }

            cmd = {
                command: 'effect',
                effect: {
                    name: effect.name,
                    args: effect.args
                },
                priority: this.server.priority
            };
            this.sendCommand(cmd);
        },

        /**
         * Sends a command for color transformation
         * @param {object} transform
         */
        setTransform: function (transform) {
            var cmd;

            if (!transform) {
                return;
            }

            cmd = {
                'command': 'transform',
                'transform': transform
            };

            this.sendCommand(cmd);
        },

        /**
         * @private
         * @param command
         */
        sendCommand: function (command) {
            var data;

            if (!command) {
                return;
            }

            if (typeof command === 'string') {
                data = command;
            } else {
                data = JSON.stringify(command);
            }

            this.socket.isConnected(function (connected) {
                if (connected) {
                    this.socket.write(data + '\n', function () {
                        this.emit('cmdSent', command);
                    }.bind(this), function (error) {
                        this.emit('error', error);
                    }.bind(this));
                } else {
                    this.emit('error', 'No server connection');
                }
            }.bind(this));
        },

        /**
         * Get the information about the hyperion server
         */
        getServerInfo: function () {
            var cmd = {command: 'serverinfo'};

            this.socket.isConnected(function (connected) {
                if (connected) {
                    this.socket.write(JSON.stringify(cmd) + '\n', function () {
                        this.socket.read(function (result) {
                            var dataobj, str = tools.ab2str(result);
                            dataobj = JSON.parse(str);
                            this.emit('serverInfo', dataobj.info);
                        }.bind(this), function (error) {
                            this.emit('error', error);
                        }.bind(this));
                    }.bind(this), function (error) {
                        this.emit('error', error);
                    }.bind(this));
                } else {
                    this.emit('error', 'No server connection');
                }
            }.bind(this));
        },

        isConnecting: function () {
            return this.connecting;
        }
    });
});
