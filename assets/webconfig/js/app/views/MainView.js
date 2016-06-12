define([
    'lib/stapes',
    'lib/tinycolor',
    'utils/Tools',
    'views/Slider'
], function (Stapes, Tinycolor, tools, Slider) {
    'use strict';
    var timer;

    function showMessageField (text, type) {
        var dom, wrapper;

        dom = document.querySelector('.work .msg');

        if (!dom) {
            dom = document.createElement('div');
            dom.classList.add('msg');
            dom.classList.add(type);
            wrapper = document.createElement('div');
            wrapper.classList.add('wrapper_msg');
            wrapper.classList.add('invisible');
            wrapper.appendChild(dom);

            document.querySelector('.work').appendChild(wrapper);
            setTimeout(function () {
                wrapper.classList.remove('invisible');
            }, 0);
        }

        if (!dom.classList.contains(type)) {
            dom.className = 'msg';
            dom.classList.add(type);
        }
        dom.innerHTML = text;
        if (timer) {
            clearTimeout(timer);
        }
        timer = setTimeout(function () {
            var error = document.querySelector('.work .wrapper_msg');
            if (error) {
                error.parentNode.removeChild(error);
            }
        }, 1600);
    }

    return Stapes.subclass(/** @lends MainView.prototype*/{
        pointer: null,
        colorpicker: null,
        slider: null,
        sliderinput: null,
        cpradius: 0,
        cpcenter: 0,
        drag: false,
        color: null,
        brightness: 1.0,
        inputbox: null,

        /**
         * @class MainView
         * @construct
         * @fires barClick
         * @fires colorChange
         */
        constructor: function () {
            var ev;
            this.pointer = document.querySelector('#colorpicker #pointer');
            this.colorpicker = document.querySelector('#colorpicker #colorwheelbg');
            this.slider = new Slider({
                element: document.getElementById('brightness'),
                min: 0,
                max: 1,
                step: 0.02,
                value: 1
            });
            this.inputbox = document.querySelector('#color input.value');

            this.cpradius = this.colorpicker.offsetWidth / 2;
            this.cpcenter = this.colorpicker.offsetLeft + this.cpradius;

            this.bindEventHandlers();

            ev = document.createEvent('Event');
            ev.initEvent('resize', true, true);
            window.dispatchEvent(ev);
        },

        /**
         * @private
         */
        bindEventHandlers: function () {
            var cptouchrect;

            window.addEventListener('resize', function () {
                var attrW, attrH, side, w = this.colorpicker.parentNode.clientWidth, h = this.colorpicker.parentNode.clientHeight;

                attrW = this.colorpicker.getAttribute('width');
                attrH = this.colorpicker.getAttribute('height');
                side = attrW === 'auto' ? attrH : attrW;
                if (w > h) {
                    if (attrH !== side) {
                        this.colorpicker.setAttribute('height', side);
                        this.colorpicker.setAttribute('width', 'auto');
                    }
                } else if (attrW !== side) {
                    this.colorpicker.setAttribute('height', 'auto');
                    this.colorpicker.setAttribute('width', side);
                }

                this.cpradius = this.colorpicker.offsetWidth / 2;
                this.cpcenter = this.colorpicker.offsetLeft + this.cpradius;
                if (this.color) {
                    this.updatePointer();
                }
            }.bind(this));

            window.addClickHandler(document.querySelector('.footer'), function (event) {
                this.emit('barClick', event.target.parentNode.dataset.area);
            }.bind(this));


            this.slider.on('changeValue', function (value) {
                this.brightness = value.value;
                this.updateInput();
                this.fireColorEvent();
            }, this);

            this.inputbox.addEventListener('input', function (event) {
                var bright, rgb = new Tinycolor(event.target.value).toRgb();

                if (rgb.r === 0 && rgb.g === 0 && rgb.b === 0) {
                    this.brightness = 0;
                    this.color = new Tinycolor({
                        r: 0xff,
                        g: 0xff,
                        b: 0xff
                    });
                } else {
                    bright = Math.max(rgb.r, rgb.g, rgb.b) / 256;
                    rgb.r = Math.round(rgb.r / bright);
                    rgb.g = Math.round(rgb.g / bright);
                    rgb.b = Math.round(rgb.b / bright);
                    this.brightness = bright;
                    this.color = new Tinycolor(rgb);
                }

                this.fireColorEvent();
                this.updatePointer();
                this.updateSlider();
            }.bind(this), false);

            this.inputbox.addEventListener('keydown', function (event) {
                switch (event.keyCode) {
                    case 8:
                    case 9:
                    case 16:
                    case 37:
                    case 38:
                    case 39:
                    case 40:
                    case 46:
                        break;
                    default:
                    {
                        if (event.target.value.length >= 6 && (event.target.selectionEnd - event.target.selectionStart) === 0) {
                            event.preventDefault();
                            event.stopPropagation();
                        } else if (event.keyCode < 48 || event.keyCode > 71) {
                            event.preventDefault();
                            event.stopPropagation();
                        }
                    }
                }
            });

            cptouchrect = document.querySelector('#colorpicker .touchrect');
            window.addPointerDownHandler(cptouchrect, function (event) {
                this.leaveInput();
                this.drag = true;
                this.handleEvent(event);
            }.bind(this));

            window.addPointerMoveHandler(cptouchrect, function (event) {
                if (this.drag) {
                    this.handleEvent(event);
                    event.preventDefault();
                }
            }.bind(this));

            window.addPointerUpHandler(cptouchrect, function () {
                this.drag = false;
            }.bind(this));

            window.addClickHandler(document.querySelector('#clear_button'), function () {
                this.emit('clear');
            }.bind(this));

            window.addClickHandler(document.querySelector('#clearall_button'), function () {
                this.emit('clearall');
            }.bind(this));
        },

        /**
         * @private
         * @param event
         */
        handleEvent: function (event) {
            var point, x, y;

            if (event.touches) {
                x = event.touches[0].clientX;
                y = event.touches[0].clientY;
            } else {
                x = event.clientX;
                y = event.clientY;
            }
            point = this.getCirclePoint(x, y);
            this.color = this.getColorFromPoint(point);

            this.updatePointer();
            this.updateSlider();
            this.updateInput();

            this.fireColorEvent();
        },

        /**
         * @private
         * @param {number} x
         * @param {number} y
         * @returns {{x: number, y: number}}
         */
        getCirclePoint: function (x, y) {
            var p = {
                x: x,
                y: y
            }, c = {
                x: this.colorpicker.offsetLeft + this.cpradius,
                y: this.colorpicker.offsetTop + this.cpradius
            }, n;

            n = Math.sqrt(Math.pow((x - c.x), 2) + Math.pow((y - c.y), 2));

            if (n > this.cpradius) {
                p.x = (c.x) + this.cpradius * ((x - c.x) / n);
                p.y = (c.y) + this.cpradius * ((y - c.y) / n);
            }

            return p;
        },

        /**
         * @private
         * @param {{x: number, y: number}} p
         * @returns {Tinycolor}
         */
        getColorFromPoint: function (p) {
            var h, t, s, x, y;
            x = p.x - this.colorpicker.offsetLeft - this.cpradius;
            y = this.cpradius - p.y + this.colorpicker.offsetTop;
            t = Math.atan2(y, x);
            h = (t * (180 / Math.PI) + 360) % 360;
            s = Math.min(Math.sqrt(x * x + y * y) / this.cpradius, 1);

            return new Tinycolor({
                h: h,
                s: s,
                v: 1
            });
        },

        /**
         * @private
         * @param color
         * @returns {{x: number, y: number}}
         */
        getPointFromColor: function (color) {
            var t, x, y, p = {};

            t = color.h * (Math.PI / 180);
            y = Math.sin(t) * this.cpradius * color.s;
            x = Math.cos(t) * this.cpradius * color.s;

            p.x = Math.round(x + this.colorpicker.offsetLeft + this.cpradius);
            p.y = Math.round(this.cpradius - y + this.colorpicker.offsetTop);

            return p;
        },

        /**
         * @private
         */
        fireColorEvent: function () {
            var rgb = this.color.toRgb();
            rgb.r = Math.round(rgb.r * this.brightness);
            rgb.g = Math.round(rgb.g * this.brightness);
            rgb.b = Math.round(rgb.b * this.brightness);
            this.emit('colorChange', rgb);
        },

        /**
         *
         * @param rgb
         */
        setColor: function (rgb) {
            var bright;

            if (rgb.r === 0 && rgb.g === 0 && rgb.b === 0) {
                this.brightness = 0;
                this.color = new Tinycolor({
                    r: 0xff,
                    g: 0xff,
                    b: 0xff
                });
            } else {
                bright = Math.max(rgb.r, rgb.g, rgb.b) / 256;
                rgb.r = Math.round(rgb.r / bright);
                rgb.g = Math.round(rgb.g / bright);
                rgb.b = Math.round(rgb.b / bright);
                this.brightness = bright;
                this.color = new Tinycolor(rgb);
            }

            this.updatePointer();
            this.updateSlider();
            this.updateInput();
        },

        /**
         * @private
         */
        updateSlider: function () {
            this.slider.setValue(this.brightness);
            this.slider.dom.style.backgroundImage = '-webkit-linear-gradient(left, #000000 0%, ' + this.color.toHexString() + ' 100%)';
        },

        /**
         * @private
         */
        updatePointer: function () {
            var point = this.getPointFromColor(this.color.toHsv());

            this.pointer.style.left = (point.x - this.pointer.offsetWidth / 2) + 'px';
            this.pointer.style.top = (point.y - this.pointer.offsetHeight / 2) + 'px';
            this.pointer.style.backgroundColor = this.color.toHexString();
        },

        /**
         * @private
         */
        updateInput: function () {
            var rgb = this.color.toRgb();
            rgb.r = Math.round(rgb.r * this.brightness);
            rgb.g = Math.round(rgb.g * this.brightness);
            rgb.b = Math.round(rgb.b * this.brightness);

            this.inputbox.value = tools.b2hexstr(rgb.r) + tools.b2hexstr(rgb.g) + tools.b2hexstr(rgb.b);
        },

        /**
         * Scroll to the specific tab content
         * @param {string} id - Id of the tab to scroll to
         */
        scrollToArea: function (id) {
            var area, index;
            document.querySelector('.footer .selected').classList.remove('selected');
            document.querySelector('.footer .button[data-area =' + id + ']').classList.add('selected');

            area = document.getElementById(id);
            index = area.offsetLeft / area.clientWidth;
            area.parentNode.style.left = (-index * 100) + '%';
        },

        /**
         * Shows a status message
         * @param {string} message - Text to show
         */
        showStatus: function (message) {
            showMessageField(message, 'status');
        },

        /**
         * Shows the error text
         * @param {string} error - Error message
         */
        showError: function (error) {
            showMessageField(error, 'error');
        },

        /**
         * @private
         */
        leaveInput: function () {
            this.inputbox.blur();
        }
    });
});
