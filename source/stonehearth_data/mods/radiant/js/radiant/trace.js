var RadiantTrace;

(function () {
   RadiantTrace = SimpleClass.extend({

      _debug : false,

      init: function(uri, properties, options) {
         this._traces = {};
         this._bubbleNotificationsUp = options ? options.bubbleNotificationsUp : false;

         if (uri) {
            this.traceUri(uri, properties);
         }
      },

      progress: function (cb) {
         this._top_deferred.progress(cb);
         return this;
      },

      fail: function (cb) {
         this._top_deferred.fail(cb);
         return this;
      },

      done: function (cb) {
         this._top_deferred.done(cb);
         return this;
      },

      always: function (cb) {
         this._top_deferred.always(cb);
         return this;
      },

      destroy: function() {
         this._destroyed = true;
         this._destroyAllTraces();
      },

      traceUri: function(uri, properties) {
         properties = properties == undefined ? {} : properties;
         this._top_deferred = this._expand_uri(uri, properties);
         return this._top_deferred;
      },

      useDeferred: function(user_deferred, properties) {
         var info = {
            trace: user_deferred,
            deferred: $.Deferred(),
            uri: '"manually created trace"',
         };
         self._recursive_trace(info, properties, level)
         return info.deferred;
      },

      _assert : function(e) {
         if (!e) {
            throw 'assertion failed!';
         }
      },

      _log : function() {
         if (!this._debug) {
            return;
         }
         var args = Array.prototype.slice.call(arguments);
         var level = args[0];
         var str = args[1]
         args.splice(0, 2); // remove 2 elements from index 0

         var indent = '';
         var level = level ? level : 0;
         for (i = 0; i < level; i++) {
            indent += '   ';
         }
         str = indent + str;
         args.splice(0, 0, level, str) // stick level and indented str at the front
         console.log.apply(console, args);
      },

      _destroyAllTraces: function() {
         $.each(this._traces, function(k, info) {
            info.trace.destroy();
         });
         this._top_deferred = null;
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

         
         if (!typeof(uri) == "string") {
            console.error("attempting to fetch something that's not a uri " + uri);   
         }
         //Ember.assert("attempting to fetch something that's not a uri " + uri.toString(), toString.call(uri) == '[object String]')

         self._log(level, 'fetching uri: ' + uri);

         // If we're already tracing this uri, just return the object that's already there.
         var info = self._get_trace_reference(uri, level);
         if (!info) {
            info = self._create_trace(uri, level);
            self._recursive_trace(info, properties, level)
         }
         return info.deferred;
      },

      _recursive_trace: function(info, properties, level) {
         var self = this;

         info.trace
            .progress(function(json) {
               // whenever we get an update, expand the entire object and yield the result            
               self._log(level, 'in progress callback for ' + info.uri  + '.  expanding...');
               if (json) {
                  self._expand_object(json, properties, level != undefined ? level + 1 : 0)
                     .progress(function(eobj) {
                        self._log(level, 'notifying deferred that uri at ' + info.uri + ' has changed');
                        info.deferred.notify(eobj);
                     })
                     .fail(function(o) {
                        self._log(level, 'expand object trace of ' + info.uri + ' failed', o);
                        info.deferred.reject(o)
                     });
               } else {
                  // should we notify or fail?
                  info.deferred.notify(null);
               }
            })
            .fail(function(o) {               
               self._log(level, 'root recursive trace of ' + info.uri + ' failed', o);
               info.deferred.reject(o);
            });
      },

      _expand_object_properties: function(obj, properties, level) {
         var deferred = $.Deferred();
         var self = this;
         var pending = {};
         var eobj = Ember.Object.create();
         var ok_to_update = false;
         var first_update = true;

         var notify_updated_again = function() {           
            if (!self._destroyed) {
               deferred.notify(eobj);
            }
         };

         var notify_update = function() {
            if (ok_to_update && Object.keys(pending).length == 0) {
               self._log(level, 'notifying parent of progress!');
               if (first_update) {
                  // this is the first update.  push the change out immediately
                  deferred.notify(eobj);
                  first_update = false;
               } else if (self._bubbleNotificationsUp) {
                  // it's changed again!  schedule a notification near the top of
                  // the runloop.  this will ensure we don't notify many many times
                  // if lots of stuff deep in the hiearchary is changing.
                  Ember.run.scheduleOnce('actions', this, notify_updated_again);
               }
            } else {
               self._log(level, 'not yet notifying parent of progress!', 'ok_to_update:', ok_to_update, 'pending_object_keys', Object.keys(pending));
            }
         };

         $.each(obj, function(k, v) {
            // Ember doesn't like periods. Transform them before using them as a key.
            // If anyone tries to use the transform value to send it back to someone, then
            // they need to do the inverse transform.
            var emberValidKey = k.replace(/\./g, "&#46;");

            var sub_properties = properties[k] || properties['*']
            if (sub_properties == undefined) {
               eobj.set(emberValidKey, v);
            } else {
               self._log(level, 'fetching child object: ' + k);
               pending[k] = true;

               var uri = ((obj instanceof Array) || (obj instanceof Object)) ? null : obj.toString();

               if (v) {
                  var update_property = function(value) {
                     self._log(level, 'child object ' + k + ' resolved to:', value);

                     // If an object already exists at this uri, make sure we destory the object.
                     var current = eobj.get(emberValidKey);
                     if (current && uri) {
                        self._release_trace_reference(uri, level);
                     }

                     // Update the object property and check to see if it's time to notify
                     // the parent
                     if (value != undefined) {
                        eobj.set(emberValidKey, value);
                     } else {
                        delete eobj[emberValidKey];
                     }
                     delete pending[k];

                     //self._log(level, 'pending is now (' + Object.keys(pending).length + ') ' + JSON.stringify(pending));
                     notify_update();
                  };
                  self._expand_object(v, sub_properties, level + 1)
                     .progress(function(child_eobj) {
                        self._log(level, 'child object ' + k + ' resolved to:', child_eobj);
                        update_property(child_eobj);
                     })
                     .fail(function(o) {
                        self._log(level, 'trace faild on child object ' + k + '.');
                        update_property(undefined);
                     })

               } else {
                  eobj.set(emberValidKey, null);
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
               var update_property = function(value) {
                  var current = earr[i];
                  if (current) {
                     current.destroy();
                  }

                  earr[i] = value;
                  pending_count--;

                  self._log(level, 'pending is now (' + pending_count + ') ');
                  notify_update();
               };
               self._expand_object(v, properties, level + 1)
                  .progress(function(child_eobj) {
                     update_property(child_eobj);
                  })
                  .fail(function(o) {
                     self._log(level, 'failed to trace array entry', i)
                     update_property(undefined);
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

   