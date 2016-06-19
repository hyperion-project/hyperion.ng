define(['lib/stapes', 'views/ServerList'], function (Stapes, ServerList) {
    'use strict';

    function createLabelInputLine (params) {
        var dom, el;

        dom = document.createElement('div');
        dom.classList.add('inputline');
        dom.id = params.id;

        el = document.createElement('label');
        el.innerHTML = params.label;
        dom.appendChild(el);

        el = document.createElement('input');
        if (typeof params.value === 'number') {
            el.type = 'number';
        }
        el.value = params.value || '';
        el.autocomplete='off';
        el.autocorrect='off';
        el.autocapitalize='off';
        dom.appendChild(el);

        return dom;
    }

    function createButtonLine (params) {
        var dom, el;

        dom = document.createElement('div');
        dom.classList.add('inputline');

        el = document.createElement('button');
        el.innerHTML = params.label;
        el.classList.add('OK');
        dom.appendChild(el);

        el = document.createElement('button');
        el.innerHTML = 'Cancel';
        el.classList.add('CANCEL');
        dom.appendChild(el);

        return dom;
    }

    function createDetectLine () {
        var dom, el;

        dom = document.createElement('div');
        dom.classList.add('line');

        el = document.createElement('button');
        el.id = 'detect_button';
        el.innerHTML = 'Detect';
        dom.appendChild(el);

        el = document.createElement('div');
        el.classList.add('spinner');
        el.classList.add('hidden');
        dom.appendChild(el);

        return dom;
    }

    return Stapes.subclass(/** @lends SettingsView.prototype */{
        dom: null,
        serverList: null,

        /**
         * @class SettingsView
         * @classdesc View for the settings
         * @constructs
         */
        constructor: function () {
            var list = [], el;

            this.dom = document.querySelector('#settings');

            this.serverList = new ServerList({
                id: 'serverList',
                label: 'Server',
                list: list
            });

            this.serverList.on({
                add: function () {
                    var line, box;

                    this.enableDetectButton(false);
                    this.lockList(true);

                    box = document.createDocumentFragment();

                    line = createLabelInputLine({id: 'name', label: 'Name:'});
                    box.appendChild(line);
                    line = createLabelInputLine({id: 'address', label: 'Address:'});
                    box.appendChild(line);
                    line = createLabelInputLine({id: 'port', label: 'Port:', value: 19444});
                    box.appendChild(line);
                    line = createLabelInputLine({id: 'priority', label: 'Priority:', value: 50});
                    box.appendChild(line);
                    line = createLabelInputLine({id: 'duration', label: 'Duration (sec):', value: 0});
                    box.appendChild(line);

                    line = createButtonLine({label: 'Add'});

                    window.addClickHandler(line.firstChild, function (event) {
                        var server = {}, i, inputs = event.target.parentNode.parentNode.querySelectorAll('input');

                        for (i = 0; i < inputs.length; i++) {
                            server[inputs[i].parentNode.id] = inputs[i].value;
                        }

                        server.port = parseInt(server.port);
                        server.priority = parseInt(server.priority);
                        server.duration = parseInt(server.duration);

                        this.emit('serverAdded', server);
                    }.bind(this));

                    window.addClickHandler(line.lastChild, function () {
                        this.emit('serverAddCanceled');
                    }.bind(this));
                    box.appendChild(line);
                    this.serverList.append(null, false, box);
                },
                select: function (id) {
                    if (!this.dom.classList.contains('locked')) {
                        this.emit('serverSelected', parseInt(id.replace('server_', '')));
                    }
                },
                remove: function (id) {
                    this.emit('serverRemoved', parseInt(id.replace('server_', '')));
                },
                edit: function (id) {
                    this.emit('editServer', parseInt(id.replace('server_', '')));
                }
            }, this);

            this.dom.appendChild(this.serverList.getDom());

            el = createDetectLine();

            window.addClickHandler(el.querySelector('button'), function () {
                this.emit('detect');
            }.bind(this));

            this.dom.appendChild(el);
        },

        /**
         * Fills the list of known servers
         * @param {Array} servers
         */
        fillServerList: function (servers) {
            var i, server, params;
            this.serverList.clear();
            for (i = 0; i < servers.length; i++) {
                server = servers[i];
                params = {id: 'server_' + i, title: server.name, subtitle: server.address + ':' + server.port};
                if (server.selected) {
                    params.selected = true;
                }
                this.serverList.addLine(params);
            }
            this.serverList.showAddButton(true);
        },

        /**
         * Shows or hides the spinner as progress indicator
         * @param {boolean} show True to show, false to hide
         */
        showWaiting: function (show) {
            if (show) {
                this.dom.querySelector('.spinner').classList.remove('hidden');
            } else {
                this.dom.querySelector('.spinner').classList.add('hidden');
            }
        },

        /**
         * Enables or disables the detect button
         * @param {Boolean} enabled True to enable, false to disable
         */
        enableDetectButton: function (enabled) {
            if (enabled) {
                this.dom.querySelector('#detect_button').classList.remove('hidden');
            } else {
                this.dom.querySelector('#detect_button').classList.add('hidden');
            }
        },

        /**
         * Locks the list for editing/deleting
         * @param {Boolean} lock True to lock, false to unlock
         */
        lockList: function (lock) {
            if (!lock) {
                this.dom.classList.remove('locked');
                this.serverList.showAddButton(true);
            } else {
                this.dom.classList.add('locked');
                this.serverList.showAddButton(false);
            }
        },

        editServer: function (serverInfo) {
            var line, box;

            this.lockList(true);
            this.enableDetectButton(false);

            box = document.createDocumentFragment();

            line = createLabelInputLine({id: 'name', label: 'Name:', value: serverInfo.server.name});
            box.appendChild(line);
            line = createLabelInputLine({id: 'address', label: 'Address:', value: serverInfo.server.address});
            box.appendChild(line);
            line = createLabelInputLine({id: 'port', label: 'Port:', value: serverInfo.server.port});
            box.appendChild(line);
            line = createLabelInputLine({id: 'priority', label: 'Priority:', value: serverInfo.server.priority});
            box.appendChild(line);
            line = createLabelInputLine({id: 'duration', label: 'Duration (sec):', value: serverInfo.server.duration});
            box.appendChild(line);

            line = createButtonLine({label: 'Done'});

            window.addClickHandler(line.querySelector('button.OK'), function (event) {
                var server = {}, i, inputs = event.target.parentNode.parentNode.querySelectorAll('input');

                for (i = 0; i < inputs.length; i++) {
                    server[inputs[i].parentNode.id] = inputs[i].value;
                }

                server.port = parseInt(server.port);
                server.priority = parseInt(server.priority);
                server.duration = parseInt(server.duration);
                if (serverInfo.server.selected) {
                    server.selected = true;
                }

                this.emit('serverChanged', {index: serverInfo.index, server: server});
            }.bind(this));

            window.addClickHandler(line.querySelector('button.CANCEL'), function () {
                this.emit('serverEditCanceled');
            }.bind(this));
            box.appendChild(line);

            window.addClickHandler(box.querySelector('input'), function (event) {
                event.stopPropagation();
            });

            this.serverList.replace(serverInfo.index, box);
        }
    });
});

