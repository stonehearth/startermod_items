var RadiantTrace;

(function () {
   RadiantTrace = SimpleClass.extend({

      _debug : true,

      init: function() {
         this._traces = {};

      },

      destroy: function() {
         this._destroyAllTraces();
      },

      traceUri: function(uri, components) {
         return this._expand_uri(uri, components);
      },      

      _assert : function(e) {
         if (!e) {
            throw 'assertion failed!';
         }
      },

      _log : function(level, str, obj) {
         if (!this._debug) {
            return;
         }

         var indent = '';
         var level = level ? level : 0;
         for (i = 0; i < level; i++) {
            indent += '   ';
         }
         str = indent + str;
         if (obj) {
            console.log(level, str, obj);
         } else {
            console.log(level, str);
         }
      },

      _destroyAllTraces: function() {
         $.each(this._traces, function(k, info) {
            info.trace.destroy();
         });
         this._traces = {};
      },


      _create_trace: function (uri, level) {
         this._log(level, 'creating new trace for ' + uri);
         this._assert(this._traces[uri] == undefined);
         var info = {
            uri: uri,
            refcount: 1,
            deferred: $.Deferred(),
            trace: radiant.trace(uri),
         };
         this._traces[uri] = info;
         return info;
      },

      _get_trace_reference: function(uri, level) {
         var info = this._traces[uri];
         if (info) {
            info.refcount += 1;
            this._log(level, 'getting existing trace for ' + uri + '(refcount:' + info.refcount + ')');
         }
         return info
      },

      _release_trace_reference: function(uri, level) {
         var info = this._traces[uri];
         this._assert(info);
         this._assert(info.uri == uri);

         info.refcount -= 1;
         self._log(level, 'releasing existing trace for ' + uri + '(refcount:' + info.refcount + ')');

         if (info.refcount == 0) {
            self._log(level, 'destroying trace for ' + uri);
            info.trace.destroy();
            delete this._traces[uri];
         }
      },

      // Given any object (object, array, string, etc) and a tree of properties of that object
      // to expand, issues all the GETs necessary to expand the entire model.  May recur.
      _expand_object: function(obj, properties, level) {
         if (obj instanceof Array) {
            return this._expand_array_properties(obj, properties, level)
         } else if (obj instanceof Object) {
            return this._expand_object_properties(obj, properties, level)
         }
         return this._expand_uri(obj.toString(), properties, level);
      },

      // Performs a GET on the specified uri.  Expects to receive back a json object which contains
      // all sorts of references to other uris.  'Properties' is a tree of objects indicating which
      // keys of the result need to be re-fetched so we can build the entire model.  This recursively
      // expands the object until all the properties have been filled in.
      _expand_uri: function(uri, properties, level) {
         var self = this;

         Ember.assert("attempting to fetch something that's not a uri " + uri.toString(), toString.call(uri) == '[object String]')
         Ember.assert(uri + ' does not appear to be a uri while fetching.', uri.indexOf('object://') == 0)

         self._log(level, 'fetching uri: ' + uri);

         // If we're already tracing this uri, just return the object that's already there.
         var info = self._get_trace_reference(uri, level);
         if (!info) {
            info = self._create_trace(uri, level);

            // wathc the object...
            info.trace.progress(function(json) {
               // whenever we get an update, expand the entire object and yield the result
               self._log(level, 'in progress callback for ' + uri + '.  expanding...');
               if (json) {
                  self._expand_object(json, properties, level != undefined ? level + 1 : 0)
                     .progress(function(eobj) {
                        self._log(level, 'notifying deferred that uri at ' + uri + ' has changed');
                        info.deferred.notify(eobj);
                     });
               } else {
                  // should we notify or fail?
                  info.deferred.notify(null);
               }
            });
         }
         return info.deferred;
      },

      _expand_object_properties: function(obj, properties, level) {
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

               var uri = ((obj instanceof Array) || (obj instanceof Object)) ? null : obj.toString();

               if (v) {
                  self._expand_object(v, properties[k], level + 1)
                     .progress(function(child_eobj) {
                        self._log(level, 'child object ' + k + ' resolved to:', child_eobj);

                        // If an object already exists at this uri, make sure we destory the object.
                        var current = eobj.get(k);
                        if (current && uri) {
                           self._release_trace_reference(uri, level);
                        }

                        // Update the object property and check to see if it's time to notify
                        // the parent
                        eobj.set(k, child_eobj);
                        delete pending[k];

                        self._log(level, 'pending is now (' + Object.keys(pending).length + ') ' + JSON.stringify(pending));
                        notify_update();
                     });
               } else {
                  eobj.set(k, null);
               }
            }
         });

         self._log(level, 'exiting _expand_object_properties');
         ok_to_update = true;
         notify_update();

         return deferred;
      },

      _expand_array_properties: function(arr, properties, level) {
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
            if (v) {
               self._expand_object(v, properties, level + 1)
                  .progress(function(child_eobj) {
                     self._log(level, 'child object ' + i + ' resolved to ' + JSON.stringify(child_eobj));
                     //earr.set(i, child_eobj);

                     var current = earr[i];
                     if (current) {
                        current.destroy();
                     }

                     earr[i] = child_eobj;
                     pending_count--;

                     self._log(level, 'pending is now (' + pending_count + ') ');
                     notify_update();
                  });
            } else {
               earr[i] = null;
            }
         });

         self._log(level, 'exiting _expand_array_properties');
         ok_to_update = true;
         notify_update();

         return deferred;
      }

   });

})();

   