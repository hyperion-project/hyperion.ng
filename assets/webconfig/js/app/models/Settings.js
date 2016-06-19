/*global define */
define(['lib/stapes', 'api/LocalStorage'], function (Stapes, LocalStorage) {
    'use strict';

    return Stapes.subclass(/** @lends Settings.prototype */{
        storage: null, servers: [],

        /**
         * @class Settings
         * @classdesc Local application settings
         * @constructs
         * @fires saved
         * @fires loaded
         * @fires error
         * @fires serverAdded
         * @fires serverChanged
         * @fires serverRemoved
         */
        constructor: function () {
            this.storage = new LocalStorage();
            this.storage.on({
                error: function (message) {
                    this.emit('error', message);
                }, got: function (settings) {
                    if (settings) {
                        this.servers = settings.servers || [];
                    }
                    this.emit('loaded');
                }, set: function () {
                    this.emit('saved');
                }
            }, this);
        },

        /**
         * Save current settings
         */
        save: function () {
            this.storage.set({
                servers: this.servers
            });
        },

        /**
         * Loads persistent settings
         */
        load: function () {
            this.storage.get();
        },

        /**
         * Add a server definition
         * @param {object} server - Server information
         */
        addServer: function (server) {
            if (this.indexOfServer(server) === -1) {
                if (this.servers.length === 0) {
                    server.selected = true;
                }

                this.servers.push(server);
                this.save();
                this.emit('serverAdded', server);
            }
        },

        /**
         * Sets a server as a default server
         * @param {number} index - Index of the server in the server list to set as default one
         */
        setSelectedServer: function (index) {
            var i;
            for (i = 0; i < this.servers.length; i++) {
                delete this.servers[i].selected;
            }
            this.servers[index].selected = true;
            this.save();
            this.emit('serverChanged', this.servers[index]);
        },

        /**
         * Remove a server from the list
         * @param {number} index - Index of the server in the list to remove
         */
        removeServer: function (index) {
            this.servers.splice(index, 1);
            this.save();
            this.emit('serverRemoved');
        },

        /**
         * Update server information
         * @param {number} index - Index of the server to update
         * @param {object} server - New server information
         */
        updateServer: function (index, server) {
            if (index >= 0 && index < this.servers.length) {
                this.servers[index] = server;
                this.save();
                this.emit('serverChanged', server);
            }
        },

        /**
         * Find the server in the list.
         * @param {object} server - Server to search index for
         * @returns {number} - Index of the server in the list. -1 if server not found
         */
        indexOfServer: function (server) {
            var i, tmp;

            for (i = 0; i < this.servers.length; i++) {
                tmp = this.servers[i];

                if (tmp.port === server.port && tmp.address === server.address) {
                    return i;
                }
            }

            return -1;
        }

    });
});
