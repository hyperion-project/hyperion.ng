/*global chrome */
chrome.app.runtime.onLaunched.addListener(function () {
    'use strict';
    chrome.app.window.create('index.html', {
        'id': 'fakeIdForSingleton',
        'innerBounds': {
            'width': 320,
            'height': 480,
            'minWidth': 320,
            'minHeight': 480
        },
        resizable: false
    });
});



