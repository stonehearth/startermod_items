
(function () {
   var Trace = SimpleClass.extend({
      init: function (uri) {
         this._uri = uri;
         this._traceInstalled = false;
         this._deferred = $.Deferred();
         this._handler = null;
      },

      progress: function (cb) {
         // install a new notification handler for when the trace is done
         // and install a new trace if we haven't done so yet.
         this._deferred.progress(cb);
         if (!this._traceInstalled) {
            this._installTrace();
            this._traceInstalled = true;
         }
         return this;
      },

      fail: function (cb) {
         this._deferred.fail(cb);
         return this;
      },

      destroy: function () {
         // remove all traces
         if (this._traceId) {
            var traceId = this._traceId;
            this._traceId = undefined;
            $.getJSON('/api/trace', { create: false, trace_id: traceId })
               .done(function(json) {
                  console.log('removed trace ' + traceId +  ' ' + JSON.stringify(json));
               })
         }

         // remove the event handler.
         if (this._handler) {
            $(top).off("radiant.events.trace_fired", this._handler);
            this._handler = null;
         }
         this._traceInstalled = false;
      },

      _installTrace: function () {
         // radiant.log('installing trace.');

         var self = this;
         $.getJSON('/api/trace', { create: true, uri: this._uri })
            .done(function (o) {
               self._traceId = o.trace_id;
               // radiant.log('trace id is ' + self._traceId);
               if (self._traceInstalled) {
                  $(top).on("radiant.events.trace_fired", function (_, e) {
                     self._onTraceFired(e.data);
                  });
                  self._onTraceFired(o);
               } else {
                  $.getJSON('/api/remove_trace', options)
                  this._traceId = null;
               }
            })
            .fail(function (err) {
               self._deferred.fail(err.statusText);
               self._deferred = null;
            })
            .always(function(o) {
               console.log('GET for trace ' + this._uri + ' finished.');
            });
      },

      _onTraceFired: function (o) {
         if (o.trace_id == this._traceId && this._deferred) {
            this._deferred.notify(o.data)
         }
      }
   });

   radiant.trace = function (uri) {
      return new Trace(uri);
   }
})();
