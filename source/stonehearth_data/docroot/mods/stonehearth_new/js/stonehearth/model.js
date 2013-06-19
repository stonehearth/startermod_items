App.RemoteObject = Ember.Object.extend({

  init: function(id, options) {
    this._super;
    var id = this.get('id');
    var components = this.get('components');

    Ember.assert("Tried to get an object without supplying { id: foo }", id != undefined);
    this._getObject(id, components)
  },

  _getObject: function(id, components) {
    var self = this;
    console.log('fetching object ' + id);
    jQuery.getJSON("http://localhost/api/objects/" + id + ".txt", function(json) {
      if(components && components.length > 0) {
        self._getComponentObjects(json, components);
      }
      self.setProperties(json);
    });
  },

  _getComponentObjects: function(json, components) {
    $.each(components, function(index, value) {
      if (json.hasOwnProperty(value))  {
        console.log(value);
        if(typeof json[value] === "string" && json[value].startsWith('mod://')) {
          // it's an url. create a new RemoteObject to follow the link, and set
          // the property from the link to that object
          var objectId = json[value].substring(6);
          json[value] = App.RemoteObject.create({ id: objectId });
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