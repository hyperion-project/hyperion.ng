/*global define */
define(['lib/stapes'], function (Stapes) {
    'use strict';
    return Stapes.subclass(/** @lends Network.prototype */{
        detectTimerId: null,

        /**
         * @class Network
         * @classdesc Empty network functions handler
         * @constructs
         */
        constructor: function () {
        },

        /**
         * Returns the list of known local interfaces (ipv4)
         * @param {function(string[])} [onSuccess] - Callback to call on success
         * @param {function(error:string)} [onError] - Callback to call on error
         */
        getLocalInterfaces: function (onSuccess, onError) {
            var ips = [], RTCPeerConnection;

            // https://developer.mozilla.org/de/docs/Web/API/RTCPeerConnection
            RTCPeerConnection = window.RTCPeerConnection || window.mozRTCPeerConnection || window.webkitRTCPeerConnection || window.msRTCPeerConnection;

            var rtc = new RTCPeerConnection({iceServers: []});
            rtc.onicecandidate = function (event) {
                var parts;

                if (this.detectTimerId) {
                    clearTimeout(this.detectTimerId);
                }

                if (event.candidate) {
                    parts = event.candidate.candidate.split(' ');
                    if (ips.indexOf(parts[4]) === -1) {
                        console.log(event.candidate);
                        ips.push(parts[4]);
                    }
                }

                this.detectTimerId = setTimeout(function () {
                    if (onSuccess) {
                        onSuccess(ips);
                    }
                }, 200);
            }.bind(this);

            rtc.createDataChannel('');
            rtc.createOffer(rtc.setLocalDescription.bind(rtc), onError);
        },

        canDetectLocalAddress: function () {
            return window.RTCPeerConnection || window.mozRTCPeerConnection || window.webkitRTCPeerConnection || window.msRTCPeerConnection;
        }
    }, true);
});
