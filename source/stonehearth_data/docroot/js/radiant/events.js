radiant.debug.log(radiant.debug.INFO, 'loading radiant.events');

(function () {
   var RadiantApi = Class.extend({

      init: function() { 
         this._endpointId = 0;
         this._pendingCmds = {};
      },

      poll: function () {
         var self = this;
         $.getJSON('/api/events', { endpoint_id: self._endpointId })
            .done(function (data) {
               $.each(data, function (_, o) {
                  if (o.type == "radiant.events.set_endpoint") {
                     self._endpointId = o.data.endpoint_id;
                  } else if (o.type == "radiant.events.pending_command_completed") {
                     var deferred = self._pendingCmds[o.data.pending_command_id]
                     if (deferred) {
                        delete self._pendingCmds[o.data.pending_command_id];
                        self._completeCommand(deferred, o.data);
                     }
                  }
                  $(top).trigger('radiant.events.*', o);
                  $(top).trigger(o.type, o);
               });
            })
            .fail(function (jqxhr, statusTxt, errorThrown) {
               console.log("poll error: (status:" + statusTxt + " error:" + errorThrown + ")");
            })
            .always(function () {
               self.poll();
            });
      },

      _completeCommand: function(deferred, o) {
         $(top).trigger('radiant.dbg.on_cmd_' + o.result, o);

         if (o.result == 'success') {
            deferred.resolve(o.response);
         } else if (o.result == 'pending') {
            this._pendingCmds[o.pending_command_id] = deferred;
         } else if (o.result == 'error') {
            deferred.reject(o.reason);
         }
      },

      cmd: function (name, options) {
         var self = this;
         var deferred = $.Deferred();

         // xxx: need a radiant.ready() function which gets called once we've created
         // a session...

         var o = $.extend({ command: name, endpoint_id: this._endpointId }, options || {});

         $(top).trigger('radiant.dbg.on_cmd', o);

         var promise = $.ajax({
            type: "POST",
            url: "/api/commands/execute",
            contentType: 'application/json',
            dataType: "json",
            processData: false,
            data: JSON.stringify(o),
         })
            .done(function (data) {
               self._completeCommand(deferred, data);
            })
            .fail(function (jqxhr, statusTxt, errorThrown) {
               deferred.reject(errorThrown);
            });
         return deferred;
      },

      execute: function (name, options) {
         var o = $.extend({ 'action': name }, options || {});
         return this.cmd('execute_action', o);
      },

      selectEntity: function (entityId) {
         return this.cmd('select_entity', { entity_id: entityId });
      }

   });
   radiant.api = new RadiantApi();
})();
