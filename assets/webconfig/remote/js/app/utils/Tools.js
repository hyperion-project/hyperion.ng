define([], function () {
    'use strict';

    return {
        /**
         * Convert a string to ArrayBuffer
         * @param {string} str String to convert
         * @returns {ArrayBuffer} Result
         */
        str2ab: function (str) {
            var i, buf = new ArrayBuffer(str.length), bufView = new Uint8Array(buf);
            for (i = 0; i < str.length; i++) {
                bufView[i] = str.charCodeAt(i);
            }
            return buf;
        },

        /**
         * Convert an array to ArrayBuffer
         * @param array
         * @returns {ArrayBuffer} Result
         */
        a2ab: function (array) {
            return new Uint8Array(array).buffer;
        },

        /**
         * Convert ArrayBuffer to string
         * @param {ArrayBuffer} buffer Buffer to convert
         * @returns {string}
         */
        ab2hexstr: function (buffer) {
            var i, str = '', ua = new Uint8Array(buffer);
            for (i = 0; i < ua.length; i++) {
                str += this.b2hexstr(ua[i]);
            }
            return str;
        },

        /**
         * Convert byte to hexstr.
         * @param {number} byte Byte to convert
         */
        b2hexstr: function (byte) {
            return ('00' + byte.toString(16)).substr(-2);
        },

        /**
         * @param {ArrayBuffer} buffer
         * @returns {string}
         */
        ab2str: function (buffer) {
            return String.fromCharCode.apply(null, new Uint8Array(buffer));
        }
    };
});
