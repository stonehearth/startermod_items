App.View = Ember.View.extend({

   //the interface to set the remote id. Call this using the set method of the view;
   //view.set('remoteId', entity.id)
   uri: "",

   //the components to retrieve from the object
   components: [],

   _subComponents: {},

   init: function() {
      this._super();

      var self = this;
      $.each(this.components, function(i, value) {
         console.log(value);
         self._subComponents[value] = {};
         
         var component = self._subComponents;
         var parts = value.split(".");
         $.each(parts, function(i, part) {
            console.log("  " + part);
            component[part] = {};
            component = component[part];
         });

         console.log(self._subComponents);

         // xxx, build the tree for more than one level of properties
         //Ember.assert("Tried to fetch a subsubproperty, which isn't supported yet", value.indexOf(".") == -1);
      });
   },

   _updatedUri: function() {
      var self = this;

      if(this.uri) {
         this._fetch(this.uri)
            .done(function(object) {
               console.log("view context set to ");
               console.log(object);
               self.set('context', object);
            });
      } else {
         self.set('context', {});
      }

   }.observes('uri'),


   _fetch: function(uri, subProperties) {
      var self = this;
      var deferred = $.Deferred();

      var _fetchSubProperties = function (json) {
         var pending = 0

         $.each(json, function(name, value) {
            var subsubProperties = self._subComponents[name];

            if (subsubProperties != undefined) {
               pending++;
               self._fetch(value, subsubProperties) 
                  .done(function (object) {
                     json[name] = object;
                     pending--;

                     if (pending == 0) {
                        deferred.resolve(json);
                     }
                  });
            }
         });

         if (pending == 0) {
            deferred.resolve(json);
         }
      }

      $.getJSON(uri)
         .done(function(json) {
            _fetchSubProperties(json);
         });

      return deferred;

   }
});
