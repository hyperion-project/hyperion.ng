/*global define */
define(['lib/stapes'], function (Stapes) {
    'use strict';
    return Stapes.subclass(/** @lends Socket.prototype */{

        /**
         * @class Socket
         * @abstract
         */
        constructor: function () {
        },

        /**
         * Create the socket
         * @param onSuccess
         * @param onError
         * @abstract
         */
        create: function (onSuccess, onError) {
        },

        /**
         * Check if a connection is opened.
         * @abstract
         */
        isConnected: function () {
            return false;
        },

        /**
         * Connect to another peer
         * @param {Object} peer Port object
         * @param {function} [onSuccess] Callback to call on success
         * @param {function(error:string)} [onError] Callback to call on error
         * @abstract
         */
        connect: function (peer, onSuccess, onError) {
        },

        /**
         * Close the current connection
         * @param {function} [onSuccess] Callback to call on success
         * @param {function(error:string)} [onError] Callback to call on error
         * @abstract
         */
        close: function (onSuccess, onError) {
        },

        /**
         * Read data from the socket
         * @param {function} [onSuccess] Callback to call on success
         * @param {function(error:string)} [onError] Callback to call on error
         * @abstract
         */
        read: function (onSuccess, onError) {
        },

        /**
         * Writes data to the socket
         * @param {string | Array} data Data to send.
         * @param {function} [onSuccess] Callback to call if data was sent successfully
         * @param {function(error:string)} [onError] Callback to call on error
         * @abstract
         */
        write: function (data, onSuccess, onError) {
        }
    }, true);
});
