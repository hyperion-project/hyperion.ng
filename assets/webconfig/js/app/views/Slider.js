define(['lib/stapes'], function (Stapes) {
    'use strict';

    function syncView (slider) {
        var left = (slider.value - slider.min) * 100 / (slider.max - slider.min);
        slider.dom.lastElementChild.style.left = left + '%';
    }

    function handleEvent(event, slider) {
        var x, left, ratio, value, steppedValue;
        if (event.touches) {
            x = event.touches[0].clientX;
        } else {
            x = event.clientX;
        }

        left = x - slider.dom.getBoundingClientRect().left;
        ratio = left / slider.dom.offsetWidth;
        value = (slider.max - slider.min) * ratio;

        steppedValue = (value - slider.min) % slider.step;
        if (steppedValue <= slider.step / 2) {
            value = value - steppedValue;
        } else {
            value = value + (slider.step - steppedValue);
        }

        value = Math.max(value, slider.min);
        value = Math.min(value, slider.max);
        slider.value = value;
        slider.emit('changeValue', {
            'value': value,
            'target': slider.dom
        });
    }

    return Stapes.subclass(/** @lends Slider.prototype */{
        /**
         * @private
         * @type {Element}
         */
        dom: null,

        /**
         * @private
         * @type {Number}
         */
        min: 0,

        /**
         * @private
         * @type {Number}
         */
        max: 100,

        /**
         * @private
         * @type {Number}
         */
        value: 0,

        /**
         * @private
         * @type {Number}
         */
        step: 1,

        /**
         * @class Slider
         * @constructs
         * @fires change
         */
        constructor: function (params) {
            this.dom = params.element;
            this.setValue(params.value);
            this.setMin(params.min);
            this.setMax(params.max);
            this.setStep(params.step);

            this.bindEventHandlers();
        },

        /**
         * @private
         */
        bindEventHandlers: function () {
            var that = this;

            function pointerMoveEventHandler(event) {
                if (that.drag) {
                    handleEvent(event, that);
                    syncView(that);
                    event.preventDefault();
                }
            }

            function pointerUpEventHandler() {
                that.drag = false;
                syncView(that);
                window.removePointerMoveHandler(document, pointerMoveEventHandler);
                window.removePointerUpHandler(document, pointerUpEventHandler);
            }

            window.addPointerDownHandler(this.dom, function (event) {
                this.drag = true;
                handleEvent(event, this);
                syncView(this);
                window.addPointerMoveHandler(document, pointerMoveEventHandler);
                window.addPointerUpHandler(document, pointerUpEventHandler);
            }.bind(this));
        },

        setValue: function(value) {
            this.value = value || 0;
            syncView(this);
        },

        setMin: function(value) {
            this.min = value || 0;
            syncView(this);
        },

        setMax: function(value) {
            this.max = value || 100;
            syncView(this);
        },

        setStep: function(value) {
            this.step = value || 1;
            syncView(this);
        }
    });
});

