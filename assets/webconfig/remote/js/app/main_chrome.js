/*global require, requirejs */

requirejs.config({
    baseUrl: '../js/app',
    paths: {
        'lib': '../vendor'
    },
    map: {
        'controllers/AppController': {
            'api/Socket': 'api/ChromeTcpSocket',
            'api/Network': 'api/ChromeNetwork'
        },
        'models/Settings': {
            'api/LocalStorage': 'api/ChromeLocalStorage'
        }
    }
});

/**
 *
 * @param {HTMLElement} dom
 * @param {function} handler
 */
window.addPointerDownHandler = function (dom, handler) {
    'use strict';
    dom.addEventListener('touchstart', function (event) {
        handler.apply(this, arguments);
        event.preventDefault();
    }, false);

    dom.addEventListener('mousedown', function () {
        handler.apply(this, arguments);
    }, false);
};

/**
 *
 * @param {HTMLElement} dom
 * @param {function} handler
 */
window.addPointerUpHandler = function (dom, handler) {
    'use strict';
    dom.addEventListener('touchend', function (event) {
        handler.apply(this, arguments);
        event.preventDefault();
    }, false);

    dom.addEventListener('mouseup', function () {
        handler.apply(this, arguments);
    }, false);
};

/**
 *
 * @param {HTMLElement} dom
 * @param {function} handler
 */
window.addPointerMoveHandler = function (dom, handler) {
    'use strict';
    dom.addEventListener('touchmove', function (event) {
        handler.apply(this, arguments);
        event.preventDefault();
    }, false);

    dom.addEventListener('mousemove', function () {
        handler.apply(this, arguments);
    }, false);
};

/**
 *
 * @param {HTMLElement} dom
 * @param {function} handler
 */
window.addClickHandler = function (dom, handler) {
    'use strict';
    var toFire = false;

    dom.addEventListener('touchstart', function (event) {
        if (event.touches.length > 1) {
            return;
        }
        toFire = true;
    }, false);

    dom.addEventListener('touchmove', function () {
        toFire = false;
    }, false);

    dom.addEventListener('touchend', function (event) {
        if (toFire) {
            handler.apply(this, arguments);
            if (event.target.tagName !== 'INPUT') {
                event.preventDefault();
            }
        }
    }, false);
	
    dom.addEventListener('click', function () {
        handler.apply(this, arguments);
    }, false);
};

require(['controllers/AppController'], function (AppController) {
    'use strict';
    var app = new AppController();
    app.init();
});
