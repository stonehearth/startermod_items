
(function () {
   var Trace = SimpleClass.extend({
      init: function (deferred) {
         var self = this;
         this._deferred = deferred;
      },

      _setCallId: function(callId) {
         this._callId = callId;
      },

      progress: function (cb) {
         this._deferred.progress(cb);
         return this;
      },

      fail: function (cb) {
         this._deferred.fail(cb);
         return this;
      },

      done: function (cb) {
         this._deferred.done(cb);
         return this;
      },

      always: function (cb) {
         this._deferred.always(cb);
         return this;
      },

      destroy: function() {
         if (this._deferred && this._callId) {
            radiant.call('radiant.remove_trace', this._callId);
            this._deferred = null;
         }
      }
   });

   var Object = SimpleClass.extend({      
      init: function () {
         this._start_poll();
         this._pendingCalls = {};

         var self = this;
         $(document).ready(function() {
            var foward_fn = function(e, o) {
               var d = self._pendingCalls[o.call_id];
               if (d) {
                  var f = {
                     'call_progress' : d.notify,
                     'call_done' : d.resolve,
                     'call_fail' : d.reject,
                  }[e.type];
                  if (f) {
                     f(o.data);
                  }
               }
            }
            $(top).on("call_progress.radiant", foward_fn);
            $(top).on("call_done.radiant", foward_fn);
            $(top).on("call_fail.radiant", foward_fn);
         });
      },

      _start_poll : function() {
         var self = this;
         self._docall('/r/call?fn=radiant.get_events&long_poll=1', []).deferred
            .done(function (data) {
               $.each(data, function (_, o) {
                  $(top).trigger(o.type, o.data);
               });
            })
            .fail(function (jqxhr, statusTxt, errorThrown) {
               console.log("poll error: (status:" + statusTxt + " error:" + errorThrown + ")");
            })
            .always(function () {
               self._start_poll();
            });
      },

      _docall : function (url, args) {
         var self = this;
         var deferred = $.Deferred();

         var request = $.ajax({
            type: 'post',
            url: url,
            contentType: 'application/json',
            data: JSON.stringify(args)
         }).fail(function(data) {
            deferred.reject(data);
         }).done(function(data) {
            if (data.type == 'call_deferred.radiant') {
               self._pendingCalls[data.call_id] = deferred;
            } else {
               deferred.resolve(data);
            }
         });
         return { deferred: deferred, request: request };
      }
   });

   var object = new Object();

   radiant.callv = function(fn, args) {
      return object._docall('/r/call/?fn='+fn, args);
   };

   radiant.call_objv = function(obj, fn, args) {
      return object._docall('/r/call/?obj='+obj + '&fn='+fn, args);
   };

   radiant.call = function () {
      var args = Array.prototype.slice.call(arguments);
      if (args.length < 1) {
         throw "radiant.call requires at least 1 argument.";
      }
      var fn = args[0];
      args = args.slice(1);
      return radiant.callv(fn, args).deferred;
   };

   radiant.call_obj = function(obj, fn, args) {
      var args = Array.prototype.slice.call(arguments);
      if (args.length < 2) {
         throw "radiant.call_obj requires at least 2 arguments.";
      }
      var obj = args[0];
      var fn = args[1];
      args = args.slice(2);
      return radiant.call_objv(obj, fn, args).deferred;
   };

   radiant.trace = function(uri) {
      var o = object._docall('/r/call/?fn=radiant.install_trace', [uri, 'ui requested trace']);
      var trace = new Trace(o.deferred);
      o.request.done(function(data) {
         trace._setCallId(data.call_id);
      });
      return trace;
   };

})();
