/*
App.ObjectFactory = Ember.Object.extend({
   getObject: function(uri, options) {
      var options = options || {};
      return this._getObject(uri, options);
   },

   _getObject: function(url, components) {
      console.log('getting object ' + url);
    
      var self = this;
      var obj = {};

      jQuery.getJSON(url, function(json) {
         console.log(json);

         if( Object.prototype.toString.call(json) === '[object Array]' ) {
            console.log('  is an array');
            obj = Ember.A(json);
         } else {
            obj = Ember.Object.create(json);
            obj.setProperties(json);

            if (components && components.length > 0) {
              self._getComponentsForObject(obj, components);
            }
         }
      });

      return obj;
  },

  _getComponentsForObject: function(obj, components) {
    $.each(components, function(index, component) {
      if (obj.hasOwnProperty(component))  {
         // it's an url. create a new RemoteObject to follow the link, and set
         // the property from the link to that object
         //var objectId = json[value].substring(6);
         console.log(component);
         var f = App.ObjectFactory.create(); // XXX this could be wasteful and totally the wrong thing
         var componentObj = f.getObject(obj[component]);
         obj.set(component, componentObj);
      }
    });
  }   
});

/*
App.RemoteObject = Ember.Object.extend(Ember.Array, {

  init: function(url, options) {
    this._super;
    var url = this.get('url');
    var components = this.get('components');

    Ember.assert("Tried to get an object without supplying { url: foo }", url != undefined);
    this._getObject(url, components)
  },

  _getObject: function(url, components) {
    console.log('fetching object ' + url);
    
    var self = this;
    
    jQuery.getJSON(url, function(json) {
      console.log(json);

      if( Object.prototype.toString.call(json) === '[object Array]' ) {
         console.log('  is an array');
      }

      if (components && components.length > 0) {
        self._getComponentObjects(json, components);
      }

      self.setProperties(json);
    });
  },

  _getComponentObjects: function(json, components) {
    var deferreds = [];

    $.each(components, function(index, value) {
      if (json.hasOwnProperty(value))  {
        console.log(value);
        if (typeof json[value] === "string") {
          // it's an url. create a new RemoteObject to follow the link, and set
          // the property from the link to that object
          //var objectId = json[value].substring(6);
          json[value] = App.RemoteObject.create({ url: json[value] });
        }
      }
    });
  },

   objectAt: function(idx) {
      if (idx > 3) {
         return undefined;
      } else {
         return idx;
      }
   },

   length: function() {
      return 3;
   }
})

if (typeof String.prototype.startsWith != 'function') {
  String.prototype.startsWith = function (str){
    return this.slice(0, str.length) == str;
  };
}
*/