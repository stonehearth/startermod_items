App.View = Ember.View.extend({

   //the components to retrieve from the object
   components: {},

   init: function() {
      this._super();
      this._traces = {};
   },

   destroy: function() {      
      this._destroyAllTraces();
      this._super();
   },

   _log : function(level, str) {
      var indent = '';
      var level = level ? level : 0;
      for (i = 0; i < level; i++) {
         indent += '   ';
      }
      // console.log(indent + str);
   },

   _updatedUri: function() {
      var self = this;

      this._destroyAllTraces();
      if (this.uri) {
         this._expand_uri(this.uri, '', this.components)
            .progress(function(eobj) {               
               console.log("view context set to ");
               console.log(JSON.stringify(eobj));
               self.set('context', eobj)
            });
      } else {
         self.set('context', {});
      }

   }.observes('uri'),

   _destroyAllTraces: function() {
      $.each(this._traces, function(k, info) {
         info.trace.destroy();
      });
      self._traces = {};
   },

   _destroyTraces: function(uri, level) {
      var self = this;
      var info = self._traces[uri];
      if (info) {
         self._log(level, '+++ destroying trace for ' + info.uri);
         info.trace.destroy();
         delete self._traces[uri];
         $.each(info.children, function(_, child_uri) {
            self._destroyTraces(child_uri, level);
         });
      }
   },

   _trace: function (uri, parent_uri, level) {
      this._destroyTraces(uri, level);
      Ember.assert('destroy traces failed to clear map', !this._traces[uri]);

      this._log(level, '+++ installing new trace at ' + uri + ' (parent:' + parent_uri + ')');

      if (parent_uri) {
         var pinfo = this._traces[parent_uri];
         Ember.assert('missing parent while creating trace', pinfo);
         if (pinfo) {
            pinfo.children.push(uri);
         }
      }

      var info = {
         uri: uri,
         trace: radiant.trace(uri),
         children: [],
      };
      this._traces[uri] = info;

      return info.trace;
   },

   // Performs a GET on the specified uri.  Expects to receive back a json object which contains
   // all sorts of references to other uris.  'Properties' is a tree of objects indicating which
   // keys of the result need to be re-fetched so we can build the entire model.  This recursively
   // expands the object until all the properties have been filled in.
   _expand_uri: function(uri, parent_uri,  properties, level) {
      var deferred = $.Deferred();
      var self = this;

      Ember.assert("attempting to fetch something that's not a uri " + uri.toString(), toString.call(uri) == '[object String]')
      Ember.assert(uri + ' does not appear to be a uri while fetching.', uri[0] == '/')

      self._log(level, 'fetching uri: ' + uri)
      var trace = self._trace(uri, parent_uri, level);

      // wathc the object...
      trace.progress(function(json) {
         // whenever we get an update, expand the entire object and yield the result
         self._log(level, 'in progress callback for ' + uri + '.  expanding...');
         self._expand_object(json, uri, properties, level != undefined ? level + 1 : 0)
            .progress(function(eobj) {
               self._log(level, 'notifying deferred that uri at ' + uri + ' has changed');
               deferred.notify(eobj);
            });
      });
      return deferred;
   },

   // Given any object (object, array, string, etc) and a tree of properties of that object
   // to expand, issues all the GETs necessary to expand the entire model.  May recur.
   _expand_object: function(obj, parent_uri, properties, level) {

      if (obj instanceof Array) {
         return this._expand_array_properties(obj, parent_uri, properties, level)
      } else if (obj instanceof Object) {
         return this._expand_object_properties(obj, parent_uri, properties, level)
      }
      return this._expand_uri(obj.toString(), parent_uri, properties, level);
   },

   _expand_object_properties: function(obj, parent_uri, properties, level) {
      var deferred = $.Deferred();
      var self = this;
      var pending = {};
      var eobj = Ember.Object.create();
      var ok_to_update = false;

      var notify_update = function() {
         if (ok_to_update && Object.keys(pending).length == 0) {
            self._log(level, 'notifying parent of progress!');
            deferred.notify(eobj);
            ok_to_update = false;
         }
      };

      $.each(obj, function(k, v) {
         if (properties[k] == undefined) {
            eobj.set(k, v);
         } else {
            self._log(level, 'fetching child object: ' + k);
            pending[k] = true;
            self._expand_object(v, parent_uri, properties[k], level)
               .progress(function(child_eobj) {
                  self._log(level, 'child object ' + k + ' resolved to ' + JSON.stringify(child_eobj));
                  eobj.set(k, child_eobj);
                  delete pending[k];

                  self._log(level, 'pending is now (' + Object.keys(pending).length + ') ' + JSON.stringify(pending));
                  notify_update();
               });
         }
      });

      self._log(level, 'at bottom of _expand_object_properties');
      ok_to_update = true;      
      notify_update();

      return deferred;
   },

   _expand_array_properties: function(arr, parent_uri, properties, level) {
      var deferred = $.Deferred();
      var self = this;      
      var earr = Ember.A();
      var pending_count = arr.length;
      var ok_to_update = false;
      earr.length = arr.length;
      var notify_update = function() {
         if (ok_to_update && pending_count == 0) {
            self._log(level, 'notifying parent of progress!');
            deferred.notify(earr);
            ok_to_update = false;
         }
      };

      $.each(arr, function(i, v) {
         self._log(level, 'fetching child object: ' + i);
         self._expand_object(v, parent_uri, properties, level)
            .progress(function(child_eobj) {
               self._log(level, 'child object ' + i + ' resolved to ' + JSON.stringify(child_eobj));
               //earr.set(i, child_eobj);
               earr[i] = child_eobj;
               pending_count--;

               self._log(level, 'pending is now (' + pending_count + ') ');
               notify_update();
            });
      });

      self._log(level, 'at bottom of _expand_array_properties');
      ok_to_update = true;
      notify_update();

      return deferred;
   },

/*
   _expand_object_properties_dead: function(obj, model, properties, level) {
      var self = this;
      var deferred = $.Deferred();
      var isArray = obj instanceof Array;
      var ok_to_notify = false;
      var pending = {};

      var notify_update = function() {
         var key_count = Object.keys(pending).length;
         self._log(level, 'checking completeness of ' + context_path + '. ok_to_notify: ' + ok_to_notify + ' key_count: ' + key_count);
         if (ok_to_notify && key_count == 0) {
            self._log(level, 'notifying parent of progress!');
            deferred.notify(obj);
         }
      };

      // Expand all the elements of the container.  In the case of the array, we just
      // recur on all the items in the array.  For the object, we take one step down
      // the properties tree for all keys that match.
      $.each(obj, function(k, v) { // here, k is either and index or a key (doesn't matter!)
         var expandProperties = isArray ? properties : properties[k];
         if (expandProperties != undefined) {
            var next_context_path = isArray ? (context_path + '[' + k + ']') : (context_path + '.' + k);
            // Yup!  Expand this one.  Increment pending so we can keep track of how
            // many outstanding requests we've issued, and expand the value.
            pending[k] = v
            self._log(level, 'fetching child object: ' + next_context_path);
            self._expand_properties(v, next_context_path, expandProperties, level)
               .progress(function(json) {
                  // Now that we've expanded, blow away whatever was in the container
                  // at the original key with the resolved object.
                  self._log(level, 'child object ' + next_context_path + ' resolved to ' + JSON.stringify(json));

                  if (self._contextValid) {
                     self._log(level, 'setting ' + next_context_path + ' to ' + JSON.stringify(json));
                     self.set(next_context_path, json);
                     var o = self.get(next_context_path);
                     self._log(level, 'value is now: ' + JSON.stringify(o));
                  } else {
                     obj[k] = json;
                     delete pending[k];

                     self._log(level, 'pending is now (' + Object.keys(pending).length + ') ' + JSON.stringify(pending));
                     notify_update();
                  }
               });
         }
      });
      ok_to_notify = true;
      self._log(level, 'at bottom of _expand_object_properties: ' + context_path)
      notify_update();

      return deferred;
   },
   */

});
