/*global define */
define(['lib/stapes'], function (Stapes) {
    'use strict';
    return Stapes.subclass(/** @lends LocalStorage.prototype */{

        /**
         * @class LocalStorage
         * @classdesc LocalStorage handler using HTML5 localStorage
         * @constructs
         *
         * @fires got
         * @fires error
         * @fires set
         */
        constructor: function () {
        },

        /**
         * Gets stored data
         */
        get: function () {
            var data;

            if (!window.localStorage) {
                this.emit('error', 'Local Storage not supported');
                return;
            }

            if (localStorage.data) {
                data = JSON.parse(localStorage.data);
                this.emit('got', data);
            }
        },

        /**
         * Stores settings
         * @param {object} data - Data object to store
         */
        set: function (data) {
            if (!window.localStorage) {
                this.emit('error', 'Local Storage not supported');
                return;
            }

            localStorage.data = JSON.stringify(data);
            this.emit('set');
        }
    });
});
