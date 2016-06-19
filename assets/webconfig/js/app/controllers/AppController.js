/*global define */
define([
    'lib/stapes', 'views/MainView', 'models/Settings', 'views/SettingsView', 'views/EffectsView', 'views/TransformView', 'data/ServerControl', 'api/Socket', 'api/Network'
], function (Stapes, MainView, Settings, SettingsView, EffectsView, TransformView, ServerControl, Socket, Network) {
    'use strict';
    var network = new Network();

    return Stapes.subclass(/** @lends AppController.prototype */{
        /**
         * @type MainView
         */
        mainView: null,
        /**
         * @type SettingsView
         */
        settingsView: null,
        /**
         * @type EffectsView
         */
        effectsView: null,
        /**
         * @type TransformView
         */
        transformView: null,
        /**
         * @type Settings
         */
        settings: null,
        /**
         * @type ServerControl
         */
        serverControl: null,
        color: {
            r: 25,
            g: 25,
            b: 25
        },
        effects: [],
        transform: {},
        selectedServer: null,

        /**
         * @class AppController
         * @constructs
         */
        constructor: function () {
            this.mainView = new MainView();
            this.settingsView = new SettingsView();
            this.effectsView = new EffectsView();
            this.transformView = new TransformView();

            this.settings = new Settings();

            this.bindEventHandlers();
            this.mainView.setColor(this.color);

            if (!network.canDetectLocalAddress()) {
                this.settingsView.enableDetectButton(false);
            }
        },

        /**
         * Do initialization
         */
        init: function () {
            this.settings.load();
        },

        /**
         * @private
         */
        bindEventHandlers: function () {
            this.settings.on({
                'loaded': function () {
                    var i;

                    for (i = 0; i < this.settings.servers.length; i++) {
                        if (this.settings.servers[i].selected) {
                            this.selectedServer = this.settings.servers[i];
                            break;
                        }
                    }

                    this.settingsView.fillServerList(this.settings.servers);

                    if (!this.selectedServer) {
                        this.gotoArea('settings');
                    } else {
                        this.connectToServer(this.selectedServer);
                    }
                },
                'error': function (message) {
                    this.showError(message);
                },
                'serverAdded': function (server) {
                    var i;
                    for (i = 0; i < this.settings.servers.length; i++) {
                        if (this.settings.servers[i].selected) {
                            this.selectedServer = this.settings.servers[i];
                            this.connectToServer(server);
                            break;
                        }
                    }

                    this.settingsView.fillServerList(this.settings.servers);
                },
                'serverChanged': function (server) {
                    var i;
                    for (i = 0; i < this.settings.servers.length; i++) {
                        if (this.settings.servers[i].selected) {
                            this.selectedServer = this.settings.servers[i];
                            this.connectToServer(server);
                            break;
                        }
                    }

                    this.settingsView.fillServerList(this.settings.servers);
                    this.connectToServer(server);
                },
                'serverRemoved': function () {
                    var i, removedSelected = true;
                    this.settingsView.fillServerList(this.settings.servers);

                    for (i = 0; i < this.settings.servers.length; i++) {
                        if (this.settings.servers[i].selected) {
                            removedSelected = false;
                            break;
                        }
                    }

                    if (removedSelected) {
                        this.selectedServer = null;
                        if (this.serverControl) {
                            this.serverControl.disconnect();
                        }
                        this.effectsView.clear();
                        this.transformView.clear();
                    }
                }
            }, this);

            this.mainView.on({
                'barClick': function (id) {
                    if (id !== 'settings') {
                        if (!this.selectedServer) {
                            this.showError('No server selected');
                        } else if (!this.serverControl) {
                            this.connectToServer(this.selectedServer);
                        }
                    }
                    this.gotoArea(id);
                },
                'colorChange': function (color) {
                    this.color = color;

                    if (!this.selectedServer) {
                        this.showError('No server selected');
                    } else if (!this.serverControl) {
                        this.connectToServer(this.selectedServer, function () {
                            this.serverControl.setColor(color, this.selectedServer.duration);
                        }.bind(this));
                    } else {
                        this.serverControl.setColor(color, this.selectedServer.duration);
                    }
                },
                'clear': function () {
                    if (!this.selectedServer) {
                        this.showError('No server selected');
                    } else if (!this.serverControl) {
                        this.connectToServer(this.selectedServer, function () {
                            this.serverControl.clear();
                        }.bind(this));
                    } else {
                        this.serverControl.clear();
                    }
                },
                'clearall': function () {
                    if (!this.selectedServer) {
                        this.showError('No server selected');
                    } else if (!this.serverControl) {
                        this.connectToServer(this.selectedServer, function () {
                            this.serverControl.clearall();
                            this.mainView.setColor({r: 0, g: 0, b: 0});
                        }.bind(this));
                    } else {
                        this.serverControl.clearall();
                        this.mainView.setColor({r: 0, g: 0, b: 0});
                    }
                }
            }, this);

            this.settingsView.on({
                'serverAdded': function (server) {
                    if (server.address && server.port) {
                        server.priority = server.priority || 50;
                        this.settings.addServer(server);
                        this.lockSettingsView(false);
                    } else {
                        this.showError('Invalid server data');
                    }
                },
                'serverAddCanceled': function () {
                    this.lockSettingsView(false);
                    this.settingsView.fillServerList(this.settings.servers);
                },
                'serverEditCanceled': function () {
                    this.lockSettingsView(false);
                    this.settingsView.fillServerList(this.settings.servers);
                },
                'serverSelected': function (index) {
                    this.lockSettingsView(false);
                    this.settings.setSelectedServer(index);
                },
                'serverRemoved': function (index) {
                    this.settings.removeServer(index);
                },
                'serverChanged': function (data) {
                    if (data.server.address && data.server.port) {
                        data.server.priority = data.server.priority || 50;
                        this.settings.updateServer(data.index, data.server);
                        this.lockSettingsView(false);
                    } else {
                        this.showError('Invalid server data');
                    }
                },
                'editServer': function (index) {
                    var server = this.settings.servers[index];
                    this.settingsView.editServer({index: index, server: server});
                },
                'durationChanged': function (value) {
                    this.settings.duration = value;
                    this.settings.save();
                },
                'detect': function () {
                    this.lockSettingsView(true);
                    this.settingsView.showWaiting(true);
                    this.searchForServer(function (server) {
                        this.settings.addServer(server);
                    }.bind(this), function () {
                        this.lockSettingsView(false);
                        this.settingsView.showWaiting(false);
                    }.bind(this));
                }
            }, this);

            this.effectsView.on({
                'effectSelected': function (effectId) {
                    if (!this.serverControl) {
                        this.connectToServer(this.selectedServer, function () {
                            this.serverControl.runEffect(this.effects[parseInt(effectId)]);
                        }.bind(this));
                    } else {
                        this.serverControl.runEffect(this.effects[parseInt(effectId)]);
                    }
                }
            }, this);

            this.transformView.on({
                'gamma': function (data) {
                    if (data.r) {
                        this.transform.gamma[0] = data.r;
                    } else if (data.g) {
                        this.transform.gamma[1] = data.g;
                    } else if (data.b) {
                        this.transform.gamma[2] = data.b;
                    }

                    if (this.serverControl) {
                        this.serverControl.setTransform(this.transform);
                    }
                },
                'whitelevel': function (data) {
                    if (data.r) {
                        this.transform.whitelevel[0] = data.r;
                    } else if (data.g) {
                        this.transform.whitelevel[1] = data.g;
                    } else if (data.b) {
                        this.transform.whitelevel[2] = data.b;
                    }

                    if (this.serverControl) {
                        this.serverControl.setTransform(this.transform);
                    }
                },
                'blacklevel': function (data) {
                    if (data.r) {
                        this.transform.blacklevel[0] = data.r;
                    } else if (data.g) {
                        this.transform.blacklevel[1] = data.g;
                    } else if (data.b) {
                        this.transform.blacklevel[2] = data.b;
                    }

                    if (this.serverControl) {
                        this.serverControl.setTransform(this.transform);
                    }
                },
                'threshold': function (data) {
                    if (data.r) {
                        this.transform.threshold[0] = data.r;
                    } else if (data.g) {
                        this.transform.threshold[1] = data.g;
                    } else if (data.b) {
                        this.transform.threshold[2] = data.b;
                    }

                    if (this.serverControl) {
                        this.serverControl.setTransform(this.transform);
                    }
                },
                'hsv': function (data) {
                    if (data.valueGain) {
                        this.transform.valueGain = data.valueGain;
                    } else if (data.saturationGain) {
                        this.transform.saturationGain = data.saturationGain;
                    }

                    if (this.serverControl) {
                        this.serverControl.setTransform(this.transform);
                    }
                }
            }, this);
        },

        /**
         * @private
         * @param id
         */
        gotoArea: function (id) {
            this.mainView.scrollToArea(id);
        },

        /**
         * @private
         * @param server
         */
        connectToServer: function (server, onConnected) {
            if (this.serverControl) {
                if (this.serverControl.isConnecting()) {
                    return;
                }
                this.serverControl.off();
                this.serverControl.disconnect();
                this.transformView.clear();
                this.effectsView.clear();
            }

            this.serverControl = new ServerControl(server, Socket);
            this.serverControl.on({
                connected: function () {
                    this.serverControl.getServerInfo();
                },
                serverInfo: function (info) {
                    var index;
                    if (!this.selectedServer.name || this.selectedServer.name.length === 0) {
                        this.selectedServer.name = info.hostname;
                        index = this.settings.indexOfServer(this.selectedServer);
                        this.settings.updateServer(index, this.selectedServer);
                        this.settingsView.fillServerList(this.settings.servers);
                    }
                    this.effects = info.effects;
                    this.transform = info.transform[0];
                    this.updateView();
                    this.showStatus('Connected to ' + this.selectedServer.name);
                    if (onConnected) {
                        onConnected();
                    }
                },
                error: function (message) {
                    this.serverControl = null;
                    this.showError(message);
                }
            }, this);
            this.serverControl.connect();
        },

        /**
         * @private
         */
        updateView: function () {
            var i, effects = [];
            if (this.effects) {
                for (i = 0; i < this.effects.length; i++) {
                    effects.push({id: i, name: this.effects[i].name});
                }
            }

            this.effectsView.clear();
            this.effectsView.fillList(effects);

            this.transformView.clear();
            this.transformView.fillList(this.transform);
        },

        /**
         * Shows the error text
         * @param {string} error - Error message
         */
        showError: function (error) {
            this.mainView.showError(error);
        },

        /**
         * Shows a message
         * @param {string} message - Text to show
         */
        showStatus: function (message) {
            this.mainView.showStatus(message);
        },

        /**
         * @private
         * @param lock
         */
        lockSettingsView: function (lock) {
            if (network.canDetectLocalAddress()) {
                this.settingsView.enableDetectButton(!lock);
            }
            this.settingsView.lockList(lock);
        },

        /**
         * @private
         * @param onFound
         * @param onEnd
         */
        searchForServer: function (onFound, onEnd) {
            network.getLocalInterfaces(function (ips) {
                if (ips.length === 0) {
                    onEnd();
                    return;
                }

                function checkInterface (localInterfaceAddress, ciOnFinished) {
                    var index, ipParts, addr;

                    index = 1;
                    ipParts = localInterfaceAddress.split('.');
                    ipParts[3] = index;
                    addr = ipParts.join('.');

                    function checkAddressRange (startAddress, count, carOnFinished) {
                        var ipParts, i, addr, cbCounter = 0, last;

                        function checkAddress (address, port, caOnFinished) {
                            var server = new ServerControl({'address': address, 'port': port}, Socket);
                            server.on({
                                'error': function () {
                                    server.disconnect();
                                    caOnFinished();
                                },
                                'connected': function () {
                                    server.getServerInfo();
                                },
                                'serverInfo': function (result) {
                                    var serverInfo = {
                                        'address': address,
                                        'port': port,
                                        'priority': 50
                                    };
                                    server.disconnect();

                                    if (result.hostname) {
                                        serverInfo.name = result.hostname;
                                    }

                                    caOnFinished(serverInfo);
                                }
                            });
                            server.connect();
                        }

                        function checkAddressDoneCb (serverInfo) {
                            var ipParts, nextAddr;

                            if (serverInfo && onFound) {
                                onFound(serverInfo);
                            }

                            cbCounter++;
                            if (cbCounter === count) {
                                ipParts = startAddress.split('.');
                                ipParts[3] = parseInt(ipParts[3]) + count;
                                nextAddr = ipParts.join('.');
                                carOnFinished(nextAddr);
                            }
                        }

                        ipParts = startAddress.split('.');
                        last = parseInt(ipParts[3]);

                        for (i = 0; i < count; i++) {
                            ipParts[3] = last + i;
                            addr = ipParts.join('.');

                            checkAddress(addr, 19444, checkAddressDoneCb);
                        }
                    }

                    function checkAddressRangeCb (nextAddr) {
                        var ipParts, count = 64, lastPart;

                        ipParts = nextAddr.split('.');
                        lastPart = parseInt(ipParts[3]);
                        if (lastPart === 255) {
                            ciOnFinished();
                            return;
                        } else if (lastPart + 64 > 254) {
                            count = 255 - lastPart;
                        }

                        checkAddressRange(nextAddr, count, checkAddressRangeCb);
                    }

                    // do search in chunks because the dispatcher used in the ios socket plugin can handle only 64 threads
                    checkAddressRange(addr, 64, checkAddressRangeCb);
                }

                function checkInterfaceCb () {
                    if (ips.length === 0) {
                        onEnd();
                    } else {
                        checkInterface(ips.pop(), checkInterfaceCb);
                    }
                }

                checkInterface(ips.pop(), checkInterfaceCb);

            }.bind(this), function (error) {
                this.showError(error);
            }.bind(this));
        }
    });
});
