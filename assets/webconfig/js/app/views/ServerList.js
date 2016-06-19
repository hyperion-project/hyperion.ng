define(['lib/stapes'], function (Stapes) {
    'use strict';

    /**
     *
     * @param params
     * @returns {HTMLElement}
     */
    function buildDom (params) {
        var dom, label, ul, li, i, header;

        dom = document.createElement('div');
        dom.classList.add('grouplist');
        dom.id = params.id;

        header = document.createElement('div');
        header.classList.add('header');
        dom.appendChild(header);
        label = document.createElement('div');
        label.innerHTML = params.label;
        label.classList.add('title');
        header.appendChild(label);
        label = document.createElement('div');
        label.innerHTML = '&#xe804;';
        label.classList.add('callout');
        header.appendChild(label);

        ul = document.createElement('ul');
        dom.appendChild(ul);
        if (params.list) {
            for (i = 0; i < params.list.length; i++) {
                li = createLine(params.list[i]);
                ul.appendChild(li);
            }
        }

        return dom;
    }

    function createLine (params) {
        var dom, el, horiz, box, touch;

        dom = document.createDocumentFragment();

        horiz = document.createElement('div');
        horiz.classList.add('horizontal');
        dom.appendChild(horiz);

        touch = document.createElement('div');
        touch.classList.add('horizontal');
        horiz.appendChild(touch);

        el = document.createElement('div');
        el.classList.add('checkbox');
        touch.appendChild(el);

        box = document.createElement('div');
        box.classList.add('titlebox');
        touch.appendChild(box);

        el = document.createElement('label');
        el.classList.add('title');
        el.innerHTML = params.title || '';
        box.appendChild(el);

        el = document.createElement('label');
        el.classList.add('subtitle');
        el.innerHTML = params.subtitle;
        box.appendChild(el);

        el = document.createElement('div');
        el.classList.add('touchrect');
        touch.appendChild(el);

        el = document.createElement('div');
        el.classList.add('edit_icon');
        el.innerHTML = '&#xe801;';
        horiz.appendChild(el);

        el = document.createElement('div');
        el.classList.add('delete_icon');
        el.innerHTML = '&#xe612;';
        horiz.appendChild(el);

        return dom;
    }

    return Stapes.subclass(/** @lends ServerList.prototype */{
        /**
         * @private
         * @type {HTMLElement}
         */
        dom: null,

        /**
         * @class ServerList
         * @constructs
         * @param {object} params - List parameter
         * @param {string} params.id - List id
         * @param {string} params.label - List title
         * @param {{title: string, subtitle: string, c}[]} [params.list] - List elements
         *
         * @fires add
         * @fires remove
         * @fires select
         * @fires edit
         */
        constructor: function (params) {
            this.dom = buildDom(params || {});
            this.bindEventHandlers();
        },

        /**
         * @private
         */
        bindEventHandlers: function () {
            window.addClickHandler(this.dom.querySelector('.callout'), function () {
                this.emit('add');
            }.bind(this));

            window.addClickHandler(this.dom.querySelector('ul'), function (event) {
                if (event.target.classList.contains('delete_icon')) {
                    this.emit('remove', event.target.parentNode.parentNode.id);
                } else if (event.target.classList.contains('edit_icon')) {
                    this.emit('edit', event.target.parentNode.parentNode.id);
                } else if (event.target.classList.contains('touchrect')) {
                    this.emit('select', event.target.parentNode.parentNode.parentNode.id);
                }
            }.bind(this));
        },

        /**
         * Returns the DOM of the list
         * @returns {HTMLElement}
         */
        getDom: function () {
            return this.dom;
        },

        /**
         * Append a line
         * @param id
         * @param selected
         * @param element
         */
        append: function (id, selected, element) {
            var li = document.createElement('li');
            li.id = id;
            if (selected) {
                li.classList.add('selected');
            }
            li.appendChild(element);
            this.dom.querySelector('ul').appendChild(li);
        },

        /**
         * Replace a line
         * @param index
         * @param element
         */
        replace: function (index, element) {
            var child = this.dom.querySelector('ul li:nth-child(' + (index + 1) + ')'), li;
            if (child) {
                li = document.createElement('li');
                li.appendChild(element);
                this.dom.querySelector('ul').replaceChild(li, child);
            }
        },

        /**
         * Add a line
         * @param lineParam
         */
        addLine: function (lineParam) {
            var line = createLine(lineParam);
            this.append(lineParam.id, lineParam.selected, line);
        },

        /**
         * Clear the list
         */
        clear: function () {
            this.dom.querySelector('ul').innerHTML = '';
        },

        /**
         * Hide or show the Add button in the header
         * @param {boolean} show - True to show, false to hide
         */
        showAddButton: function (show) {
            if (show) {
                this.dom.querySelector('.callout').classList.remove('invisible');
            } else {
                this.dom.querySelector('.callout').classList.add('invisible');
            }
        }
    });
});

