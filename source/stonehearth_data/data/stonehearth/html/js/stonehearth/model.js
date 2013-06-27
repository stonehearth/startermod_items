App.RemoteObject = Ember.Object.extend({

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
      if (components && components.length > 0) {
        self._getComponentObjects(json, components);
      }

      var patchedJson = {};

      for (key in json) {
         var patchedKey = key.replace(".",":");
         patchedJson[patchedKey] = json[key];
         //console.log(patchedKey + ' = ' + json[key])
      }

      self.setProperties(patchedJson);
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
  }



})

if (typeof String.prototype.startsWith != 'function') {
  String.prototype.startsWith = function (str){
    return this.slice(0, str.length) == str;
  };
}