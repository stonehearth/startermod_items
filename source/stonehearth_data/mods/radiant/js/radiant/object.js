
(function () {
   var Tracer = SimpleClass.extend({
      init: function(o, uri, tracerMap) {
         this._uri = uri;
         this._traces = [];
         this._tracerMap = tracerMap;
         this._dead_traces = [];

         var self = this;
         o.request.done(function(data) {
            this._callId = data.call_id;
         });

         o.deferred.done(function(data) {
            self._result = data;
            self._protectTraceList(function() {
               _.each(self._traces, function (trace) {
                  trace._deferred.resolve(data);
               });
            });
         });
         o.deferred.fail(function(data) {
            self._error = data;
            self._protectTraceList(function() {
               _.each(self._traces, function (trace) {
                  trace._deferred.reject(data);
               });
            });
         });
         o.deferred.progress(function(data) {
            self._progress = data;
            self._protectTraceList(function() {
               _.each(self._traces, function (trace) {
                  trace._deferred.notify(data);
               });
            });
         });
      },

      _protectTraceList: function(fn) {
         var self = this;
         
         this._protect_traces = true;
         fn();
         this._protect_traces = false;

         _.each(self._dead_traces, function (trace) {
            self.removeTrace(trace);
         });
         this._dead_traces = []
      },

      addTrace: function(trace) {
         this._traces.push(trace);
         if (this._result) {
            trace._deferred.resolve(this._result);
         } else if (this._error) {
            trace._deferred.reject(this._error);
         } else if (this._progress) {
            trace._deferred.notify(this._progress);
         }
         return trace;
      },

      removeTrace: function(trace) {
         if (this._protect_traces) {
            this._dead_traces.push(trace);
            return;
         }
         var offset = this._traces.indexOf(trace);
         radiant.assert(offset >= 0, 'trace not found in remove_trace');
         this._traces.splice(offset, 1); // remove it
         if (_.isEmpty(this._traces)) {
            radiant.call('radiant:remove_trace', this._callId);
            delete this._tracerMap[this.uri];
         }
      }
   });

   var Trace = SimpleClass.extend({
      init: function (tracer) {
         this._deferred = $.Deferred();
         this._tracer = tracer;
         this._tracer.addTrace(this);
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
         if (this._tracer) {
            this._tracer.removeTrace(this);
            this._tracer = undefined;
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
                     'radiant_call_progress' : d.notify,
                     'radiant_call_done' : d.resolve,
                     'radiant_call_fail' : d.reject
                  }[e.type];
                  if (f) {
                     f(o.data);
                  }
               }
            };
            $(top).on("radiant_call_progress", foward_fn);
            $(top).on("radiant_call_done", foward_fn);
            $(top).on("radiant_call_fail", foward_fn);
         });
      },

      _start_poll : function() {
         var self = this;
         var deferred = self._docall('/r/call?fn=radiant:get_events&long_poll=1', []).deferred
         deferred
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
            radiant.report_error('low level call ' + url + ' failed.', data);
            deferred.reject(data);
         }).done(function(data) {
            if (data.type == 'radiant_call_deferred') {
               self._pendingCalls[data.call_id] = deferred;
            } else {
               deferred.resolve(data);
            }
         });
         return { deferred: deferred, request: request };
      }
   });

   var object = new Object();

   radiant._tracerMap = {};   

   radiant.callv = function(fn, args) {
      return object._docall('/r/call/?fn='+fn, args);
   };

   radiant.call_objv = function(obj, fn, args) {
      return object._docall('/r/call/?obj='+obj + '&fn='+fn, args);
   };

   radiant.call = function () {
      var args = Array.prototype.slice.call(arguments);
      if (args.length < 1) {
         throw "radiant:call requires at least 1 argument.";
      }
      var fn = args[0];
      args = args.slice(1);
      return radiant.callv(fn, args).deferred;
   };

   radiant.call_obj = function() {
      var args = Array.prototype.slice.call(arguments);
      if (args.length < 2) {
         throw "radiant:call_obj requires at least 2 arguments.";
      }
      var obj = args[0];
      var fn = args[1];
      args = args.slice(2);
      return radiant.call_objv(obj, fn, args).deferred;
   };

   radiant.trace = function(uri) {
      var tracer = this._tracerMap[uri];
      if (tracer == undefined) {
         var o = object._docall('/r/call/?fn=radiant:install_trace', [uri, 'ui requested trace']);
         tracer = new Tracer(o, uri, this._tracerMap);
         this._tracerMap[uri] = tracer;
      }
      return new Trace(tracer);
   };

})();
