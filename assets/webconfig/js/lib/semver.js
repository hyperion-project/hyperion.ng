(function webpackUniversalModuleDefinition(root, factory) {
	if(typeof exports === 'object' && typeof module === 'object')
		module.exports = factory();
	else if(typeof define === 'function' && define.amd)
		define([], factory);
	else if(typeof exports === 'object')
		exports["semverLite"] = factory();
	else
		root["semverLite"] = factory();
})(this, function() {
return /******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId]) {
/******/ 			return installedModules[moduleId].exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			i: moduleId,
/******/ 			l: false,
/******/ 			exports: {}
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.l = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// define getter function for harmony exports
/******/ 	__webpack_require__.d = function(exports, name, getter) {
/******/ 		if(!__webpack_require__.o(exports, name)) {
/******/ 			Object.defineProperty(exports, name, {
/******/ 				configurable: false,
/******/ 				enumerable: true,
/******/ 				get: getter
/******/ 			});
/******/ 		}
/******/ 	};
/******/
/******/ 	// getDefaultExport function for compatibility with non-harmony modules
/******/ 	__webpack_require__.n = function(module) {
/******/ 		var getter = module && module.__esModule ?
/******/ 			function getDefault() { return module['default']; } :
/******/ 			function getModuleExports() { return module; };
/******/ 		__webpack_require__.d(getter, 'a', getter);
/******/ 		return getter;
/******/ 	};
/******/
/******/ 	// Object.prototype.hasOwnProperty.call
/******/ 	__webpack_require__.o = function(object, property) { return Object.prototype.hasOwnProperty.call(object, property); };
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(__webpack_require__.s = 0);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var SemverVersion = __webpack_require__(1);

var semver = {
    version: '0.0.5',
    SemverVersion: SemverVersion,
    validate: function validate(version) {
        return SemverVersion.validate(version);
    },
    compare: function compare(a, b, needCompareBuildVersion) {
        return new SemverVersion(a).compare(new SemverVersion(b), needCompareBuildVersion);
    },
    format: function format(version) {
        return new SemverVersion(version).format();
    },
    instance: function instance(version) {
        return new SemverVersion(version);
    },
    compareMainVersion: function compareMainVersion(a, b) {
        return new SemverVersion(a).compareMainVersion(new SemverVersion(b));
    },
    gt: function gt(a, b, needCompareBuildVersion) {
        var result = this.compare(a, b, needCompareBuildVersion);
        return result === 1;
    },
    gte: function gte(a, b, needCompareBuildVersion) {
        var result = this.compare(a, b, needCompareBuildVersion);
        return result === 1 || result === 0;
    },
    lt: function lt(a, b, needCompareBuildVersion) {
        var result = this.compare(a, b, needCompareBuildVersion);
        return result === -1;
    },
    lte: function lte(a, b, needCompareBuildVersion) {
        var result = this.compare(a, b, needCompareBuildVersion);
        return result === -1 || result === 0;
    },
    equal: function equal(a, b, needCompareBuildVersion) {
        var result = this.compare(a, b, needCompareBuildVersion);
        return result === 0;
    },
    equalMain: function equalMain(a, b) {
        return new SemverVersion(a).mainVersion === new SemverVersion(b).mainVersion;
    },

    // 主版本转成数字类型方便比较
    mainVersionToNumeric: function mainVersionToNumeric(version) {
        var digit = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : 6;

        var semverVersion = new SemverVersion(version);
        return semverVersion.mainVersionToNumeric(digit);
    }
};

module.exports = semver;

/***/ }),
/* 1 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var MAX_LENGTH = 256;
var MAX_SAFE_INTEGER = Number.MAX_SAFE_INTEGER || 9007199254740991;

// 正则标识
// 数字，禁止纯数字补 0 
var NUMERIC_IDENTIFIER = '0|[1-9]\\d*';
// 数字，字母，横线
var NUMERIC_LETTERS_IDENTIFIER = '[0-9A-Za-z-]*';
var BUILD_IDENTIFIER = '[0-9A-Za-z-]+';
// 数字和字母组合，达到禁止纯数字补0的目的
var NON_NUMERIC_IDENTIFIER = '\\d*[a-zA-Z-][a-zA-Z0-9-]*';
var MAIN_VERSION_IDENTIFIER = '(' + NUMERIC_IDENTIFIER + ')\\.(' + NUMERIC_IDENTIFIER + ')\\.(' + NUMERIC_IDENTIFIER + ')';
// 先行版本号，由 ASCII 码的英数字和连接号 [0-9A-Za-z-] 组成，
// 且“禁止 MUST NOT ”留白。数字型的标识符号“禁止 MUST NOT ”在前方补零
var PRERELEASE_IDENTIFIER = '(?:' + NUMERIC_IDENTIFIER + '|' + NON_NUMERIC_IDENTIFIER + ')';
var PRERELEASE = '(?:\\-(' + PRERELEASE_IDENTIFIER + '(?:\\.' + PRERELEASE_IDENTIFIER + ')*))';
// 编译版本号
var BUILD = '(?:\\+(' + BUILD_IDENTIFIER + '(?:\\.' + BUILD_IDENTIFIER + ')*))';
var FULL_VERSION_IDENTIFIER = '^v?' + MAIN_VERSION_IDENTIFIER + PRERELEASE + '?' + BUILD + '?$';

// 根据正则标识实例化正则
var REGEX_MAIN_VERSION = new RegExp(MAIN_VERSION_IDENTIFIER);
var REGEX_FULL_VERSION = new RegExp(FULL_VERSION_IDENTIFIER);
var REGEX_NUMERIC = /^[0-9]+$/;

var SemverVersion = function () {
    function SemverVersion(version) {
        _classCallCheck(this, SemverVersion);

        if (version instanceof SemverVersion) {
            return version;
        } else if (typeof version !== 'string') {
            throw new TypeError('Invalid Version: ' + version);
        }

        if (version.length > MAX_LENGTH) {
            throw new TypeError('version is longer than ' + MAX_LENGTH + ' characters');
        }

        if (!(this instanceof SemverVersion)) {
            return new SemverVersion(version);
        }

        var matches = version.trim().match(REGEX_FULL_VERSION);

        this.rawVersion = version;
        this.major = +matches[1];
        this.minor = +matches[2];
        this.patch = +matches[3];

        this._isThrowVersionNumericError(this.major, 'major');
        this._isThrowVersionNumericError(this.minor, 'minor');
        this._isThrowVersionNumericError(this.patch, 'patch');

        if (matches[4]) {
            this.prereleaseArray = matches[4].split('.').map(function (id) {
                if (REGEX_NUMERIC.test(id)) {
                    var num = +id;
                    if (num >= 0 && num < MAX_SAFE_INTEGER) {
                        return num;
                    }
                }
                return id;
            });
        } else {
            this.prereleaseArray = [];
        }

        //this.build = matches[5] ? matches[5].split('.') : [];

        this.prerelease = matches[4];
        this.build = matches[5];
        this.mainVersion = [this.major, this.minor, this.patch].join('.');
        this.version = this.mainVersion + (this.prerelease ? '-' + this.prerelease : '') + (this.build ? '+' + this.build : '');
    }

    _createClass(SemverVersion, [{
        key: '_isThrowVersionNumericError',
        value: function _isThrowVersionNumericError(versionNumber, versionName) {
            if (versionNumber > MAX_SAFE_INTEGER || this.major < 0) {
                throw new TypeError('Invalid ' + versionName + ' version');
            }
        }
    }, {
        key: '_isNumeric',
        value: function _isNumeric(numeric) {
            return REGEX_NUMERIC.test(numeric);
        }
    }, {
        key: '_padNumber',
        value: function _padNumber(num, fill) {
            var length = ('' + num).length;
            return Array(fill > length ? fill - length + 1 || 0 : 0).join(0) + num;
        }
    }, {
        key: 'mainVersionToNumeric',
        value: function mainVersionToNumeric(digit) {
            var numericStr = [this._padNumber(this.major, digit), this._padNumber(this.minor, digit), this._padNumber(this.patch, digit)].join('');
            return parseInt(numericStr);
        }
    }, {
        key: 'compare',
        value: function compare(other) {
            var needCompareBuildVersion = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : false;

            var otherSemver = other;
            if (!(other instanceof SemverVersion)) {
                otherSemver = new SemverVersion(other);
            }
            var result = this.compareMainVersion(otherSemver) || this.comparePreReleaseVersion(otherSemver);
            if (!result && needCompareBuildVersion) {
                return this.compareBuildVersion(otherSemver);
            } else {
                return result;
            }
        }

        // 比较数字

    }, {
        key: 'compareNumeric',
        value: function compareNumeric(a, b) {
            return a > b ? 1 : a < b ? -1 : 0;
        }
    }, {
        key: 'compareIdentifiers',
        value: function compareIdentifiers(a, b) {
            var aIsNumeric = this._isNumeric(a);
            var bIsNumeric = this._isNumeric(b);
            if (aIsNumeric && bIsNumeric) {
                a = +a;
                b = +b;
            }
            // 字符比数字大
            if (aIsNumeric && !bIsNumeric) {
                return -1;
            } else if (bIsNumeric && !aIsNumeric) {
                return 1;
            } else {
                return this.compareNumeric(a, b);
            }
        }
    }, {
        key: 'compareMainVersion',
        value: function compareMainVersion(otherSemver) {
            return this.compareNumeric(this.major, otherSemver.major) || this.compareNumeric(this.minor, otherSemver.minor) || this.compareNumeric(this.patch, otherSemver.patch);
        }
    }, {
        key: 'comparePreReleaseVersion',
        value: function comparePreReleaseVersion(otherSemver) {
            if (this.prereleaseArray.length && !otherSemver.prereleaseArray.length) {
                return -1;
            } else if (!this.prereleaseArray.length && otherSemver.prereleaseArray.length) {
                return 1;
            } else if (!this.prereleaseArray.length && !otherSemver.prereleaseArray.length) {
                return 0;
            }
            var i = 0;
            do {
                var a = this.prereleaseArray[i];
                var b = otherSemver.prereleaseArray[i];
                if (a === undefined && b === undefined) {
                    return 0;
                } else if (b === undefined) {
                    return 1;
                } else if (a === undefined) {
                    return -1;
                } else if (a === b) {
                    continue;
                } else {
                    return this.compareIdentifiers(a, b);
                }
            } while (++i);
        }
    }, {
        key: 'compareBuildVersion',
        value: function compareBuildVersion(otherSemver) {
            if (this.build && !otherSemver.build) {
                return 1;
            } else if (!this.build && otherSemver.build) {
                return -1;
            } else {
                return this.compareIdentifiers(this.build, otherSemver.build);
            }
        }
    }], [{
        key: 'validate',
        value: function validate(version) {
            return REGEX_FULL_VERSION.test(version);
        }
    }]);

    return SemverVersion;
}();

module.exports = SemverVersion;

/***/ })
/******/ ]);
});