/*
Ember.ObjectProxy = Ember.Object.extend( {

  content: null,
  _contentDidChange: Ember.observer(function() {
    Ember.assert("Can't set ObjectProxy's content to itself", this.get('content') !== this);
  }, 'content'),

  isTruthy: Ember.computed.bool('content'),

  _debugContainerKey: null,

  willWatchProperty: function (key) {
    var contentKey = 'content.' + key;
    addBeforeObserver(this, contentKey, null, contentPropertyWillChange);
    addObserver(this, contentKey, null, contentPropertyDidChange);
  },

  didUnwatchProperty: function (key) {
    var contentKey = 'content.' + key;
    removeBeforeObserver(this, contentKey, null, contentPropertyWillChange);
    removeObserver(this, contentKey, null, contentPropertyDidChange);
  },

  unknownProperty: function (key) {
    var content = get(this, 'content');
    if (content) {
      return get(content, key);
    }
  },

  setUnknownProperty: function (key, value) {
    var content = get(this, 'content');
    Ember.assert(fmt("Cannot delegate set('%@', %@) to the 'content' property of object proxy %@: its 'content' is undefined.", [key, value, this]), content);
    return set(content, key, value);
  }
});

Ember.ObjectProxy.reopenClass({
  create: function () {
    var mixin, prototype, i, l, properties, keyName;
    if (arguments.length) {
      prototype = this.proto();
      for (i = 0, l = arguments.length; i < l; i++) {
        properties = arguments[i];
        for (keyName in properties) {
          if (!properties.hasOwnProperty(keyName) || keyName in prototype) { continue; }
          if (!mixin) mixin = {};
          mixin[keyName] = null;
        }
      }
      if (mixin) this._initMixins([mixin]);
    }
    return this._super.apply(this, arguments);
  }
});

})();
*/

Ember.DistributedObjectProxy = Ember.ObjectProxy.extend(/** @scope Ember.DistributedObjectProxy.prototype */ {
  /**
    The object whose properties will be forwarded.

    @property content
    @type Ember.Object
    @default null
  */
  content: null,
  
  _contentDidChange: Ember.observer(function() {
    Ember.assert("Can't set ObjectProxy's content to itself", this.get('content') !== this);
  }, 'content'),

  isTruthy: Ember.computed.bool('content'),

  _debugContainerKey: null,

  willWatchProperty: function (key) {
    var contentKey = 'content.' + key;
    addBeforeObserver(this, contentKey, null, contentPropertyWillChange);
    addObserver(this, contentKey, null, contentPropertyDidChange);
  },

  didUnwatchProperty: function (key) {
    var contentKey = 'content.' + key;
    removeBeforeObserver(this, contentKey, null, contentPropertyWillChange);
    removeObserver(this, contentKey, null, contentPropertyDidChange);
  },

  unknownProperty: function (key) {
    var content = get(this, 'content');
    if (content) {
      return get(content, key);
    }
  },

  setUnknownProperty: function (key, value) {
    var content = get(this, 'content');
    Ember.assert(fmt("Cannot delegate set('%@', %@) to the 'content' property of object proxy %@: its 'content' is undefined.", [key, value, this]), content);
    return set(content, key, value);
  }
});

Ember.ObjectProxy.reopenClass({
  create: function () {
    var mixin, prototype, i, l, properties, keyName;
    if (arguments.length) {
      prototype = this.proto();
      for (i = 0, l = arguments.length; i < l; i++) {
        properties = arguments[i];
        for (keyName in properties) {
          if (!properties.hasOwnProperty(keyName) || keyName in prototype) { continue; }
          if (!mixin) mixin = {};
          mixin[keyName] = null;
        }
      }
      if (mixin) this._initMixins([mixin]);
    }
    return this._super.apply(this, arguments);
  }
});

})();

(function() {

/**
  `Ember.ObjectController` is part of Ember's Controller layer. A single shared
  instance of each `Ember.ObjectController` subclass in your application's
  namespace will be created at application initialization and be stored on your
  application's `Ember.Router` instance.

  `Ember.ObjectController` derives its functionality from its superclass
  `Ember.ObjectProxy` and the `Ember.ControllerMixin` mixin.

  @class ObjectController
  @namespace Ember
  @extends Ember.ObjectProxy
  @uses Ember.ControllerMixin
**/

//Ember.ObjectController = Ember.ObjectProxy.extend(Ember.ControllerMixin);

Ember.DistributedObjectController = Ember.DistributedObjectProxy.extend(Ember.ControllerMixin);

})();