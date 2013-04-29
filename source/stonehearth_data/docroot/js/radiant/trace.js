
(function () {
   var TraceEntities = Class.extend({
      init: function (entity_id) {
         this._entity_id = entity_id;
         this._filters = [];
         this._collectors = [];
         this._traceInstalled = false;
         this._container = {};
         this._deferred = $.Deferred();
         this._handler = null;
      },

      filter: function (query) {
         // restrict the search of entites to
         this._filters.push(query);
         return this;
      },

      get: function (property) {
         this._collectors.push(property);
         return this;
      },

      getEntityList: function (property, collectors) {
         this._collectors.push({
            type: 'entity_list',
            property: property,
            collectors: collectors.slice(0)
         })
         return this;
      },

      getEntityMap: function (property, collectors) {
         return this;
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
            radiant.api.cmd('remove_trace', { trace_id: this._traceId })
            this._traceId = null;
         }

         // remove the event handler.
         if (this._handler) {
            $(top).off("radiant.events.trace_fired", this._handler);
            this._handler = null;
         }
         this._traceInstalled = false;
      },

      _installTrace: function () {
         radiant.log('installing trace.');

         var self = this;
         var options = {
            filters: this._filters,
            collectors: this._collectors,
         }
         var cmd;
         if (this._entity_id) {
            options.entity_id = this._entity_id;
            cmd = radiant.api.cmd('trace_entity', options)
         } else {
            cmd = radiant.api.cmd('trace_entities', options)
         }

         cmd
            .done(function (res) {
               self._traceId = res.trace_id;
               radiant.log('trace id is ' + self._traceId);
               if (self._traceInstalled) {
                  self._handler = function (_, e) {
                     var o = e.data;
                     //radiant.log('got trace fired (' + o.trace_id + ' vs. ' + self._traceId + ')');
                     self._onTraceFired(o);
                  }
                  $(top).on("radiant.events.trace_fired", self._handler);
                  self._onTraceFired(res);
               } else {
                  radiant.api.cmd('remove_trace', { trace_id: this._traceId })
                  this._traceId = null;
               }
            })
            .fail(function (err) {
               self._deferred.fail(err.statusText);
               self._deferred = null;
            });
      },

      _onTraceFired: function (o) {
         if (o.trace_id == this._traceId && this._deferred) {
            this._container = $.extend({}, o.data);
            this._deferred.notify(this._container)
         }
      }
   });

   radiant.trace_entity = function (entity_id) {
      return new TraceEntities(entity_id);
   }

   radiant.trace_entities = function () {
      return new TraceEntities();
   }

})();
