/*global define, chrome */
define(['api/LocalStorage'], function (LocalStorage) {
    'use strict';
    return LocalStorage.subclass(/** @lends ChromeLocalStorage.prototype */{

        /**
         * @class ChromeLocalStorage
         * @classdesc Chrome's persistent storage
         * @constructs
         * @extends LocalStorage
         */
        constructor: function () {
        },

        get: function () {
            chrome.storage.local.get('data', function (entry) {
                if (chrome.runtime.lastError) {
                    this.emit('error', chrome.runtime.lastError.message);
                } else {
                    this.emit('got', entry.data);
                }
            }.bind(this));
        },

        set: function (data) {
            var entry = {};
            entry.data = data;
            chrome.storage.local.set(entry, function () {
                if (chrome.runtime.lastError) {
                    this.emit('error', chrome.runtime.lastError.message);
                } else {
                    this.emit('set');
                }
            }.bind(this));
        }
    });
});
