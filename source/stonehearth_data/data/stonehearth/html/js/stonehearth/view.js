App.View = Ember.View.extend({

   //the interface to set the remote id. Call this using the set method of the view;
   //view.set('remoteId', entity.id)
   uri: "",

   //the components to retrieve from the object
   components: {},

   _updatedUri: function() {
      var self = this;

      if(this.uri) {
         this._fetch(this.uri, this.components, 0)
            .done(function(object) {
               console.log("view context set to ");
               console.log(JSON.stringify(object));
               self.set('context', object);
            });
      } else {
         self.set('context', {});
      }

   }.observes('uri'),


   // Performs a GET on the specified uri.  Expects to receive back a json object which contains
   // all sorts of references to other uris.  'Properties' is a tree of objects indicating which
   // keys of the result need to be re-fetched so we can build the entire model.  This recursively
   // expands the object until all the properties have been filled in.
   _fetch: function(uri, properties) {
      var deferred = $.Deferred();
      var self = this;

      Ember.assert("attempting to fetch something that's not a uri " + uri.toString(), toString.call(uri) == '[object String]')
      Ember.assert(uri + ' does not appear to be a uri while fetching.', uri[0] == '/')

      // Grab the json at the uri.
      $.getJSON(uri)
         .done(function(json) {
            // We got it!  Now expand all the keys in this object.  This may recursively
            // expand all the keys in those objects, and so-on (and so-on (and so-on)).
            // When all that is done, _expand_properties will resolve the fully expanded
            // object.
            self._expand_properties(json, properties)
               .done(function() {
                  // Now just notify the original caller and we're done!
                  deferred.resolve(json);
               });
         });
      return deferred;
   },

   // Given any object (object, array, string, etc) and a tree of properties of that object
   // to expand, issues all the GETs necessary to expand the entire model.  May recur.
   _expand_properties: function(obj, properties) {
      var self = this;
      var deferred = $.Deferred();
      var pending = 0;

      if (obj instanceof Array || obj instanceof Object) {
         // Expand all the elements of the container.  In the case of the array, we just
         // recur on all the items in the array.  For the object, we take one step down
         // the properties tree for all keys that match.
         var isArray = obj instanceof Array
         $.each(obj, function(k, v) { // here, k is either and index or a key (doesn't matter!)
            var expandProperties = isArray ? properties : properties[k];
            if (expandProperties != undefined) {
               // Yup!  Expand this one.  Increment pending so we can keep track of how
               // many outstanding requests we've issued, and expand the value.
               pending++;
               self._expand_properties(v, expandProperties)
                  .done(function(json) {
                     // Now that we've expanded, blow away whatever was in the container
                     // at the original key with the resolved object.
                     obj[k] = json
                     
                     console.log('  setting ' + k + '[' + json + ']');
                     //console.log(JSON.stringify(object));

                     // On the way in we counted how many objects in the container needed
                     // to get resolved.  Now (on the way out), if this is the very last
                     // one, resolve the original object, since it's now been fully expanded.
                     pending--;
                     if (pending <= 0 && deferred) {
                        deferred.resolve(obj);
                     }
                  });
            }
         });
      } else {
         // must be a string!  we got here because someone put a path to this string
         // in the properties to expand object.  Therefore, this must be a uri to
         // a resource.  Either that or the call is smoking crack and has created an
         // invalid property expandion object.
         var uri = obj.toString();
         Ember.assert(uri + ' does not appear to be a uri while fetching.', uri[0] == '/');

         // Increment pending here so we don't resolve until we've had a chance
         // to fetch and fully expand the uri.
         pending++;
         self._fetch(uri, properties)
            .done(function(json) {
               deferred.resolve(json)
            });
      }

      // If there was no work to do at all, we need to resolve right away.
      if (pending == 0) {
         deferred.resolve(obj);
      }
      return deferred;
   },

   _fetch_old: function(uri, subProperties) {
      var self = this;
      var deferred = $.Deferred();
      var pending = 0;

      Ember.assert("attempting to fetch something that's not a uri " + uri.toString(), toString.call(uri) == '[object String]')
      console.log('entering fetch... (pending ' + pending + ')');

      var _fetchSubProperties = function (json) {

         var onSubFetchFinished = function() {
            pending--;
            console.log('checking resolve... currently pending ' + pending);
            if (pending == 0 && deferred != null) {
               deferred.resolve(json);
               deferred = null;
            }
         }

         $.each(json, function(name, value) {
            console.log('checking ' + name);
            var subsubProperties = subProperties[name];

            if (subsubProperties != undefined) {
               console.log('  fetching ' + name + ' ' + pending + ' currently pending');
               console.log(value);
               if (value instanceof Array || value instanceof Object) {
                  json[name] = [];
                  $.each(value, function(k, v) { // here, k is either and index or a key (doesn't matter!)
                     var key = k;
                     pending++;
                     self._fetch(v, subsubProperties)
                        .done(function(object) {
                           json[name][key] = object
                           console.log('  setting ' + name + '[' + key + ']');
                           console.log(JSON.stringify(object));
                           onSubFetchFinished();
                        });
                  });
               } else {
                  pending++;
                  var uri = value.toString();
                  Ember.assert(value + ' does not appear to be a uri while fetching.', uri[0] == '/')
                  self._fetch(uri, subsubProperties) 
                     .done(function (object) {
                        json[name] = object;
                        console.log('  setting ' + name);
                        console.log(JSON.stringify(object));
                        onSubFetchFinished();
                     });                  
               }
            }
         });
         if (pending == 0) {
            deferred.resolve(json);
            deferred = null;
         }
      }

      $.getJSON(uri)
         .done(function(json) {
            _fetchSubProperties(json);
         });

      return deferred;

   }
});
