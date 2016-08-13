/**
 * hyperion remote
 * MIT License
 */

define([
    'lib/stapes',
    'views/Slider'
], function (Stapes, Slider) {
    'use strict';

    function onHeaderClick (event) {
        var list = event.target.parentNode.parentNode.querySelector('ul');

        if (list.clientHeight === 0) {
            list.style.maxHeight = list.scrollHeight + 'px';
            event.target.parentNode.parentNode.setAttribute('collapsed', 'false');
        } else {
            list.style.maxHeight = 0;
            event.target.parentNode.parentNode.setAttribute('collapsed', 'true');
        }
    }

    function createLine (id, type, icon, caption, value, min, max) {
        var dom, el, el2, label, wrapper;

        dom = document.createElement('li');
        dom.className = type;

        label = document.createElement('label');
        label.innerHTML = caption;
        dom.appendChild(label);

        wrapper = document.createElement('div');
        wrapper.classList.add('wrapper');
        wrapper.id = id;

        el = document.createElement('div');
        el.classList.add('icon');
        el.innerHTML = icon;
        wrapper.appendChild(el);

        el = document.createElement('div');
        el.classList.add('slider');
        el2 = document.createElement('div');
        el2.classList.add('track');
        el.appendChild(el2);
        el2 = document.createElement('div');
        el2.classList.add('thumb');
        el.appendChild(el2);
        el.dataset.min = min;
        el.dataset.max = max;
        el.dataset.value = value;
        el.dataset.step = 0.01;
        wrapper.appendChild(el);

        el = document.createElement('input');
        el.classList.add('value');
        el.type = 'number';
        el.min = min;
        el.max = max;
        el.step = 0.01;
        el.value = parseFloat(Math.round(value * 100) / 100).toFixed(2);
        wrapper.appendChild(el);

        dom.appendChild(wrapper);

        return dom;
    }

    function createGroup (groupInfo) {
        var group, node, subnode, i, member;
        group = document.createElement('div');
        group.classList.add('group');
        if (groupInfo.collapsed) {
            group.setAttribute('collapsed', 'true');
        }
        group.id = groupInfo.id;

        node = document.createElement('div');
        node.classList.add('header');
        group.appendChild(node);
        subnode = document.createElement('label');
        subnode.innerHTML = groupInfo.title;
        node.appendChild(subnode);
        subnode = document.createElement('label');
        subnode.innerHTML = groupInfo.subtitle;
        node.appendChild(subnode);

        node = document.createElement('ul');
        group.appendChild(node);
        for (i = 0; i < groupInfo.members.length; i++) {
            member = groupInfo.members[i];
            subnode = createLine(member.id, member.type, member.icon, member.label, member.value, member.min,
                member.max);
            node.appendChild(subnode);
        }

        return group;
    }

    return Stapes.subclass(/** @lends TransformView.prototype */{
        sliders: {},

        /**
         * @class TransformView
         * @constructs
         */
        constructor: function () {
        },

        /**
         * Clear the list
         */
        clear: function () {
            document.querySelector('#transform .values').innerHTML = '';
        },

        /**
         * @private
         * @param change
         */
        onSliderChange: function (event) {
            var data = {}, idparts, value;

            idparts = event.target.parentNode.id.split('_');
            value = parseFloat(Math.round(parseFloat(event.value) * 100) / 100);

            event.target.parentNode.querySelector('.value').value = value.toFixed(2);

            data[idparts[1]] = value;
            this.emit(idparts[0], data);
        },

        /**
         * @private
         * @param change
         */
        onValueChange: function (event) {
            var data = {}, idparts, value;

            idparts = event.target.parentNode.id.split('_');
            value = parseFloat(Math.round(parseFloat(event.target.value) * 100) / 100);

            if (parseFloat(event.target.value) < parseFloat(event.target.min)) {
                event.target.value = event.target.min;
            } else if (parseFloat(event.target.value) > parseFloat(event.target.max)) {
                event.target.value = event.target.max;
            }
            this.sliders[event.target.parentNode.id].setValue(value);

            data[idparts[1]] = value;
            this.emit(idparts[0], data);
        },

        /**
         * fill the list
         * @param {object} transform - Object containing transform information
         */
        fillList: function (transform) {
            var dom, group, els, i, slider;

            if (!transform) {
                document.querySelector('#transform .info').classList.remove('hidden');
                return;
            }

            dom = document.createDocumentFragment();

            group = createGroup({
                collapsed: true,
                id: 'HSV',
                title: 'HSV',
                subtitle: 'HSV color corrections',
                members: [
                    {
                        id: 'hsv_saturationGain',
                        type: 'saturation',
                        icon: '&#xe806;',
                        label: 'Saturation gain',
                        value: transform.saturationGain,
                        min: 0,
                        max: 5
                    },
                    {
                        id: 'hsv_valueGain',
                        type: 'value',
                        icon: '&#xe805;',
                        label: 'Value gain',
                        value: transform.valueGain,
                        min: 0,
                        max: 5
                    }
                ]
            });
            dom.appendChild(group);

            group = createGroup({
                collapsed: true,
                id: 'Gamma',
                title: 'Gamma',
                subtitle: 'Gamma correction',
                members: [
                    {
                        id: 'gamma_r',
                        type: 'red',
                        icon: '&#xe800;',
                        label: 'Red',
                        value: transform.gamma[0],
                        min: 0,
                        max: 5
                    },
                    {
                        id: 'gamma_g',
                        type: 'green',
                        icon: '&#xe800;',
                        label: 'Green',
                        value: transform.gamma[1],
                        min: 0,
                        max: 5
                    },
                    {
                        id: 'gamma_b',
                        type: 'blue',
                        icon: '&#xe800;',
                        label: 'Blue',
                        value: transform.gamma[2],
                        min: 0,
                        max: 5
                    }
                ]
            });
            dom.appendChild(group);

            group = createGroup({
                collapsed: true,
                id: 'Whitelevel',
                title: 'Whitelevel',
                subtitle: 'Value when RGB channel is fully on',
                members: [
                    {
                        id: 'whitelevel_r',
                        type: 'red',
                        icon: '&#xe800;',
                        label: 'Red',
                        value: transform.whitelevel[0],
                        min: 0,
                        max: 1
                    },
                    {
                        id: 'whitelevel_g',
                        type: 'green',
                        icon: '&#xe800;',
                        label: 'Green',
                        value: transform.whitelevel[1],
                        min: 0,
                        max: 1
                    },
                    {
                        id: 'whitelevel_b',
                        type: 'blue',
                        icon: '&#xe800;',
                        label: 'Blue',
                        value: transform.whitelevel[2],
                        min: 0,
                        max: 1
                    }
                ]
            });
            dom.appendChild(group);

            group = createGroup({
                collapsed: true,
                id: 'Blacklevel',
                title: 'Blacklevel',
                subtitle: 'Value when RGB channel is fully off',
                members: [
                    {
                        id: 'blacklevel_r',
                        type: 'red',
                        icon: '&#xe800;',
                        label: 'Red',
                        value: transform.blacklevel[0],
                        min: 0,
                        max: 1
                    },
                    {
                        id: 'blacklevel_g',
                        type: 'green',
                        icon: '&#xe800;',
                        label: 'Green',
                        value: transform.blacklevel[1],
                        min: 0,
                        max: 1
                    },
                    {
                        id: 'blacklevel_b',
                        type: 'blue',
                        icon: '&#xe800;',
                        label: 'Blue',
                        value: transform.blacklevel[2],
                        min: 0,
                        max: 1
                    }
                ]
            });
            dom.appendChild(group);

            group = createGroup({
                collapsed: true,
                id: 'Threshold',
                title: 'Threshold',
                subtitle: 'Threshold for a channel',
                members: [
                    {
                        id: 'threshold_r',
                        type: 'red',
                        icon: '&#xe800;',
                        label: 'Red',
                        value: transform.threshold[0],
                        min: 0,
                        max: 1
                    },
                    {
                        id: 'threshold_g',
                        type: 'green',
                        icon: '&#xe800;',
                        label: 'Green',
                        value: transform.threshold[1],
                        min: 0,
                        max: 1
                    },
                    {
                        id: 'threshold_b',
                        type: 'blue',
                        icon: '&#xe800;',
                        label: 'Blue',
                        value: transform.threshold[2],
                        min: 0,
                        max: 1
                    }
                ]
            });
            dom.appendChild(group);

            els = dom.querySelectorAll('.slider');
            for (i = 0; i < els.length; i++) {
                slider = new Slider({
                    element: els[i],
                    min: els[i].dataset.min,
                    max: els[i].dataset.max,
                    step: els[i].dataset.step,
                    value: els[i].dataset.value
                });
                slider.on('changeValue', this.onSliderChange, this);
                this.sliders[els[i].parentNode.id] = slider;
            }

            els = dom.querySelectorAll('input');
            for (i = 0; i < els.length; i++) {
                els[i].addEventListener('input', this.onValueChange.bind(this), false);
            }

            els = dom.querySelectorAll('.header');
            for (i = 0; i < els.length; i++) {
                window.addClickHandler(els[i], onHeaderClick);
            }

            document.querySelector('#transform .info').classList.add('hidden');
            document.querySelector('#transform .values').appendChild(dom);
        }

    });
});

